#include "debugger.h"
#include "strconv.h"
#include <iostream>

namespace debugger
{
    using namespace std;
    using namespace strconv;

    vector<DllInfo> dllList; // ���� ���� ��ü�� �ν��Ͻ�ȭ
   
    PcCollectionManager pcManager; // pcCollection ������ ������ ������ ���� pcCollectionManager Ŭ���� �ν��Ͻ�ȭ

    // pcCollection ���Ϳ� pc �����͸� �Է��ϴ� �޼���
    void PcCollectionManager::pushPcInfo(PVOID pc, DWORD threadId)
    {
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.push({ pc, threadId }); // ������ pc�� threadID�� PcInfo ����ü ���Ϳ� ����
    }

    // pcCollection ���Ϳ��� �����ϰ� pc ������ �о���� �޼���
    PcInfo PcCollectionManager::getPcInfo()
    {
        PcInfo temp = pcCollection.front();
        lock_guard<mutex> lock(pc_mtx);
        pcCollection.pop();
       
        return temp;
    }

    bool PcCollectionManager::isQueueEmpty()
    {
        return pcCollection.empty();
    }

    Debug::Debug(tstring  cmdLine)
    {
        // �޸� �ʱ�ȭ
        cbNeeded = NULL;
        ZeroMemory(&debugEvent, sizeof(debugEvent)); // ����� �̺�Ʈ �޸� �ʱ�ȭ
        ZeroMemory(hMods, sizeof(hMods)); // �ڵ� �迭 �ʱ�ȭ
        ZeroMemory(&modInfo, sizeof(modInfo)); // ��� ���� ����ü �ʱ�ȭ
        ZeroMemory(&si, sizeof(STARTUPINFO)); // si ����ü �޸� �ʱ�ȭ
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)); // pi ����ü �޸� �ʱ�ȭ

        // �⺻�� �Ҵ�
        si.cb = sizeof(STARTUPINFO); // �⺻�� ������� �ʱ�ȭ

        // ������� ���μ��� ����
        if (!CreateProcess(NULL, const_cast<LPWSTR>(tstringToLPWSTR(cmdLine)), NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS, NULL, NULL, &si, &pi))
        {
            fprintf(stderr, "Error creating process: %d\n", GetLastError());
            throw runtime_error("Error creating process");
        }
        hProcess = pi.hProcess;
    }

    Debug::~Debug() // Debug Ŭ���� �Ҹ���
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        //sharedObj.reset(); // ���� ��ü ����
    }

    // step-by-step ������ ���� context�� trap flag ����
    void Debug::SetTrapFlag(HANDLE hThread)
    {
        DWORD threadID = GetThreadId(hThread);
        CONTEXT ctx = { 0 };

        if ((threadID != NULL) && (contextMap.find(threadID) == contextMap.end()))
        {
            ctx.ContextFlags = CONTEXT_CONTROL; // context��  control register ����

            if (!GetThreadContext(hThread, &ctx)) // ���� ���ؽ�Ʈ�� �ҷ���
            {
                fprintf(stderr, "Failed to get thread context: %d\n", GetLastError());
                return;
            }

            ctx.EFlags |= 0x100; // TF, Trap Flag ����

            if (!SetThreadContext(hThread, &ctx)) // TF�� ������ ���ؽ�Ʈ ������Ʈ
            {
                fprintf(stderr, "Failed to set thread context: %d\n", GetLastError());
            }

            contextMap.emplace(threadID, hThread); // ������ ���ؽ�Ʈ ����
        }
    }

    void Debug::pushPcInfoAndDllListByTrapFlagLoop(atomic_bool* isDebuggerOn)
    {
        TCHAR szModName[MAX_PATH];
        CONTEXT ctx = { 0 };

        while (continueDebugging && WaitForDebugEvent(&debugEvent, INFINITE))
        {
            *isDebuggerOn = true;
            switch (debugEvent.dwDebugEventCode)
            {
                // trap flag ���� �� �ش� case�� �ɸ�
            case EXCEPTION_DEBUG_EVENT:
                if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) // trap flag  exception Ȯ�ν�
                {
                    auto context = contextMap.find(debugEvent.dwThreadId); // �ش� threadID �˻�
                    if (context != contextMap.end())
                    {
                        ctx.ContextFlags = CONTEXT_CONTROL;
                        if (GetThreadContext(context->second, &ctx)) // map�� ����� �ڵ��� context�� ������
                        {
                            pcManager.pushPcInfo((PVOID)ctx.Rip, debugEvent.dwThreadId);
                            ctx.EFlags |= 0x100; // TF ����
                            SetThreadContext(context->second, &ctx); // context �ε�
                        }
                    }
                }
                break;

                // ���μ���,������ ���� �̺�Ʈ �߻� ��
            case CREATE_PROCESS_DEBUG_EVENT:
            case CREATE_THREAD_DEBUG_EVENT:
            {
                if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
                {
                    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                    {
                        if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))
                            && GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
                        {
                            dllList.push_back({ szModName, modInfo.lpBaseOfDll, modInfo.SizeOfImage });
                            tcout << szModName << endl;
                        }
                    }
                }
                SetTrapFlag(debugEvent.u.CreateThread.hThread); // ù context TF ����
            }
            break;

            case EXIT_PROCESS_DEBUG_EVENT:
                continueDebugging = FALSE; // �����(����) ���� 
                break;
            }
            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
        }
        printf("collecting pc finished...\n");
        dllLogFile.close();
        *isDebuggerOn = false;
    }
}
















