import os
from flask import Flask, render_template, request, jsonify, url_for, g
from werkzeug.utils import secure_filename
from PIL import Image
import subprocess
import win32gui
import win32ui

app = Flask(__name__)

USER_UPLOADED_FILE_FOLDER = os.path.abspath('uploads')
USER_UPLOADED_FILE_ICON_FOLDER = os.path.abspath(os.path.join('static', 'icons'))

if not os.path.exists(USER_UPLOADED_FILE_FOLDER):
    os.makedirs(USER_UPLOADED_FILE_FOLDER)

if not os.path.exists(USER_UPLOADED_FILE_ICON_FOLDER):
    os.makedirs(USER_UPLOADED_FILE_ICON_FOLDER)

app.config['USER_UPLOADED_FILE_FOLDER'] = USER_UPLOADED_FILE_FOLDER
app.config['USER_UPLOADED_FILE_ICON_FOLDER'] = USER_UPLOADED_FILE_ICON_FOLDER

def extract_icon_from_file(exe_path):
    icon_list, _ = win32gui.ExtractIconEx(exe_path, 0, 1)
    if icon_list:
        icon = icon_list[0]
        hdc = win32ui.CreateDCFromHandle(win32gui.GetDC(0))
        hdc_mem = hdc.CreateCompatibleDC()

        bmp = win32ui.CreateBitmap()
        bmp.CreateCompatibleBitmap(hdc, 32, 32)
        hdc_mem.SelectObject(bmp)
        hdc_mem.DrawIcon((0, 0), icon)

        bmpinfo = bmp.GetInfo()
        bmpstr = bmp.GetBitmapBits(True)
        extracted_icon = Image.frombuffer(
            'RGBA',
            (bmpinfo['bmWidth'], bmpinfo['bmHeight']),
            bmpstr, 'raw', 'BGRA', 0, 1
        )

        win32gui.DestroyIcon(icon)
        hdc_mem.DeleteDC()
        hdc.DeleteDC()
        return extracted_icon

class FileHandler:
    def __init__(self):
        self.file = None
        self.filename = None
        self.file_path = None

    def load_file(self):
        if 'file' not in request.files:
            return {'error': 'No file part'}, 400
        
        self.file = request.files['file']
        
        if self.file.filename == '':
            return {'error': 'No selected file'}, 400
        
        self.filename = secure_filename(self.file.filename)
        self.file_path = os.path.join(app.config['USER_UPLOADED_FILE_FOLDER'], self.filename)
        return None, 200

    def save_file(self):
        if self.file and self.file_path:
            self.file.save(self.file_path)
            return {'file uploaded': True}, 200
        return {'error': 'No file to save'}, 500

    def extract_icon(self):
        if self.file_path and self.filename:
            icon_filename = f"{os.path.splitext(self.filename)[0]}.png"
            icon_path = os.path.join(app.config['USER_UPLOADED_FILE_ICON_FOLDER'], icon_filename)
            icon_image = extract_icon_from_file(self.file_path)
            icon_image.save(icon_path)
            image_url = url_for('static', filename=f'icons/{icon_filename}')
            return {'image_url': image_url}, 200
        return {'error': 'No file path to extract icon from'}, 500

    def analyze_file(self):
        if self.file_path:
            process = subprocess.Popen([
                "PCTracer\\PCTracer.exe", 
                '-t', self.file_path, 
                '-d', 'PCTracer\\DLL.db',
                '-l', '2'
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, bufsize=1)

            stdout_lines = []
            stderr_lines = []

            for stdout_line in iter(process.stdout.readline, ''):
                stdout_lines.append(stdout_line)

            for stderr_line in iter(process.stderr.readline, ''):
                stderr_lines.append(stderr_line)

            process.stdout.close()
            process.stderr.close()
            process.wait()

            stdout_output = ''.join(stdout_lines)
            stderr_output = ''.join(stderr_lines)

            if stderr_output:
                return {'output': stderr_output}, 500
            else:
                return {'output': stdout_output}, 200
        return {'error': 'No file path to analyze'}, 500
    
@app.route('/')
def index():
    return render_template('index.html')

@app.before_request
def before_request():
    if 'file' in request.files:
        g.file_handler = FileHandler()
        error, status = g.file_handler.load_file()
        if status != 200:
            return jsonify(error), status
    else:
        g.file_handler = None

@app.route('/upload_file', methods=['POST'])
def upload_file():
    response, status = g.file_handler.save_file()
    return jsonify(response), status

@app.route('/get_file_icon_url', methods=['POST'])     
def get_file_icon_url():
    response, status = g.file_handler.extract_icon()
    return jsonify(response), status

@app.route('/analyze_file', methods=['POST'])
def analyze_file():
    response, status = g.file_handler.analyze_file()
    return jsonify(response), status


if __name__ == '__main__':
    app.run(debug=True)