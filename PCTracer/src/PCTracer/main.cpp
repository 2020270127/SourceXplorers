#include "debugger.h"
#include "record.h"
#include "cmdparser.h"
#include "cmdparser.h"
#include <thread>

using namespace cmdutil;
using namespace debugger;
using namespace record;

shared_ptr<Debug> sharedDebugger; // ������� ���� ������ ���� shared_ptr ���, ������ ���� ������ ����ñ���

// �ɼ� ���
void configure_parser(CmdParser& parser)
{
	parser.set_required<tstring>(_T("t"), _T("target_path"), _T("The target's path"));
	parser.set_required<tstring>(_T("d"), _T("db_path"), _T("The DB's path"));
	parser.set_optional<int>(_T("l"), _T("log_level"), 1, _T("log_level: TEXT for 1, DB for 2"));
};

// ���� ���
void print_hep_message(CmdParser& parser)
{
	tcout << parser.getHelpMessage(_T("PC_TRACER"));
};

int _tmain(int argc, TCHAR* argv[])
{
	CmdParser cmdParser;
	configure_parser(cmdParser);
	cmdParser.parseCmdLine(argc, argv);

	if (cmdParser.isPrintHelp())
	{
		print_hep_message(cmdParser);
	}
	else
	{
        try
        {
            tstring target_path = cmdParser.get<tstring>(_T("t"));
            tstring db_path = cmdParser.get<tstring>(_T("d"));
            int log_level = cmdParser.get<int>(_T("l"));
            atomic_bool isDebuggerOn = true;

            //record ��ü ����, ���� �����忡�� ����
            RECORD recorder = RECORD(db_path, log_level); // log_level : 1 -> text, else -> sqlite3 db
            
            // ����� ������ ���� �� ����
            thread debugThread([&target_path, &isDebuggerOn]() {
                sharedDebugger = make_shared<Debug>(target_path);
                sharedDebugger->pushPcInfoAndDllListByTrapFlagLoop(&isDebuggerOn); // pc�� �̿��Ͽ� DB���� �Լ� �˻� ����
            });
            
            recorder.logDLLInfoWhileFlagIsOn(&isDebuggerOn); // pc�� �̿��Ͽ� DB���� �Լ� �˻� ����
            debugThread.join(); // �����尡 �Ϸ�� ������ ��ٸ�
        }
        catch (const exception& e)
        {
            cerr << "An error occurred: " << e.what() << endl;
        }
	}
	return 0;
};