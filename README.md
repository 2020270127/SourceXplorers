# SourceXplorers PCTracer Server

A Windows-only Flask web application that lets you:

1. **Upload** a PE file (`.exe` or `.dll`)
2. **Extract** and display its icon
3. **Analyze** which Windows API calls it uses via the pre-built PCTracer binary

---

## Prerequisites

- **Windows 10/11 (x64)**
- **Python 3.7+**
- **PCTracer** binaries shipped in `server/PCTracer/`
- Install Python dependencies:
  ```bat
  pip install Flask Pillow pywin32
  ```

---

## Repository Layout

```
.
├── .gitignore
├── README.md                   ← (this file)
└── server/
    ├── main.py                 # Flask application entry point
    ├── readme                  # (optional) server-local notes
    ├── PCTracer/               # Pre-built tracing tools & DB
    │   ├── PCTracer.exe
    │   ├── DLLParser.exe
    │   ├── Debugger.lib
    │   └── sqlite3.lib
    ├── static/                 # Static assets
    │   ├── styles.css
    │   └── icons/              # Extracted icons
    ├── templates/
    │   └── index.html          # UI template
    └── uploads/                # Uploaded PE files (created at runtime)
```

---

## Quick Start

1. **Install dependencies**
   ```bat
   cd server
   python -m venv venv
   venv\Scripts\activate
   pip install Flask Pillow pywin32
   ```

2. **Run the server**
   ```bat
   python main.py
   ```
   By default, it listens on `http://127.0.0.1:5000`.

3. **Use the UI**
   Open `http://127.0.0.1:5000` in your browser:
   - Click **Upload** to select a PE file
   - View its extracted icon
   - Click **Analyze** to run PCTracer and display API-call output

---

## Server API Reference

### POST `/upload_file`

- **Form field**: `file` (binary)
- **Response**:
  - `200 OK` → `{ "file uploaded": true }`
  - `4xx` → `{ "error": "..." }`

### POST `/get_file_icon_url`

- **Response**:
  - `200 OK` → `{ "image_url": "/static/icons/<filename>.png" }`
  - `500` → `{ "error": "..." }`

### POST `/analyze_file`

- **Response**:
  - `200 OK` → `{ "output": "<PCTracer stdout>" }`
  - `500` → `{ "output": "<error or stderr>" }`

---

## Notes

- The server must run with **Administrator** privileges to allow PCTracer to attach to processes.
- Uploaded files are stored under `server/uploads/`.
- Extracted icons are saved under `server/static/icons/`.
- PCTracer is invoked as:
  ```bat
  server\PCTracer\PCTracer.exe -t <path_to_uploaded_file> -d server\PCTracer\DLL.db -l 2
  ```

---

## License

This project is licensed under MIT. See [LICENSE](LICENSE) in the repository root.
