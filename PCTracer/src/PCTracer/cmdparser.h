#pragma once

#include "typedef.h"
#include <stdexcept>

namespace cmdutil
{
    using namespace std;

    enum OPTION_INFO_TYPE
    {
        NAME = 0,
        ALTERNATIVE,
        DESCRIPTION
    };

    class CmdBase
    {
    protected:
        bool required_;         // �ʼ� �ɼ� ����
        bool isSet_;            // �ɼ� ���� �����Ǿ��� �� ����
        tstring name_;          // �ɼ� �̸�
        tstring alternative_;   // �ɼ� ��Ī
        tstring description_;   // �ɼ� ����

    public:
        CmdBase(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            required_(required), isSet_(false), name_(name), alternative_(alternative), description_(description) {};
        virtual ~CmdBase(void) {};

        virtual bool compare(const tstring& name);
        virtual bool isRequired(void);
        virtual bool isSet(void);
        virtual tstring getOptionInfo(const OPTION_INFO_TYPE& type);
    };

    template<typename T>
    class CmdOption : public CmdBase
    {
    private:
        T value_;

    public:
        CmdOption(const tstring& name, const tstring& alternative, const tstring& description, bool required) :
            CmdBase(name, alternative, description, required) {};
        virtual ~CmdOption(void) {};

        // ���ø� �Լ��� ��� ����� ������ ���� ���ԵǾ�� ��
        // hpp ������ �̿��ؼ� ������ �и��ؼ� �ۼ��� ���� ������ ������ �ڵ� �������� ���Ͻ�ų �� ����

        T get(void) { return value_; };
        void set(T value)
        {
            value_ = value;
            isSet_ = true;
        };
    };

    class CmdParser
    {
    private:
        bool help_;
        vector<CmdBase*> command_;

    public:
        CmdParser(void) : help_(false) {};
        virtual ~CmdParser(void) { clear(); };

        bool hasOptions(void);
        bool isRequiredOptionSet(void);
        bool isPrintHelp(void);
        void clear(void);
        vector<CmdBase*>::iterator find(const tstring& name);
        void parseCmdLine(int argc, TCHAR* argv[]);
        tstring getHelpMessage(const tstring& program);

        // ���ø� �Լ��� ��� ����� ������ ���� ���ԵǾ�� ��
        // hpp ������ �̿��ؼ� ������ �и��ؼ� �ۼ��� ���� ������ ������ �ڵ� �������� ���Ͻ�ų �� ����

        // �ʼ� �ɼ� ���
        template<typename T>
        void set_required(const tstring& name, const tstring& alternative, const tstring& description = "")
        {
            command_.push_back(dynamic_cast <CmdBase*>(new CmdOption<T>(name, alternative, description, true)));
        };

        // ���� �ɼ� ���
        template<typename T>
        void set_optional(const tstring& name, const tstring& alternative, T defaultValue, const tstring& description = "")
        {
            CmdOption<T>* cmd_option = new CmdOption<T>(name, alternative, description, false);
            cmd_option->set(defaultValue);

            command_.push_back(dynamic_cast <CmdBase*>(cmd_option));
        };

        // �ɼǿ� ������ �� ���
        template<typename T>
        T get(const tstring& name)
        {
            vector<CmdBase*>::iterator iter = find(name);
            if (iter != command_.end())
            {
                return (dynamic_cast <CmdOption<T> *>(*iter))->get();
            }
            else
            {
                throw runtime_error("The option does not exist!");
            }
        };
    };
};

