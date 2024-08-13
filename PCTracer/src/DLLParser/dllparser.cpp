#include "dllparser.h"



using namespace peparser;
using namespace std;



void dllParser::processDLL(const wstring& dllPath, const string& tableName)
{
    PEParser* peParser = new PEParser();
    if (peParser->open(dllPath.c_str()))
    {
        PEPrint* pePrint = new PEPrint(tableName);
        if (peParser->parseEATCustom())
        {
            cout << "Parsing " << tableName << "..." << endl;
            pePrint->printEAT(peParser->getPEStructure());
        }
        else
        {
            cout << "Parsing " << tableName << " failed" << endl;
        }
        delete pePrint;
    }
    else
    {
        wcout << "Opening " << dllPath << " Failed" << endl;
    }
    delete peParser;
}

void dllParser::ParseDllRecursively(const wstring& directoryPath)
{
    wstring filePath; // dll ���
    string fileName; // sql ���̺� ������ dll �̸�

    try
    {
        for (const auto& entry : filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == L".dll")
            {
                filePath = entry.path().wstring();
                fileName = entry.path().stem().string(); // ���ϸ��� Ȯ���ڸ� ������ �̸��� ����
                processDLL(filePath, fileName);
            }
        }
    }
    catch (const filesystem::filesystem_error& e)
    {
        wcerr << L"Filesystem error: " << e.what() << endl;
    }
    catch (const exception& e)
    {
        wcerr << L"General error: " << e.what() << endl;
    }
}


