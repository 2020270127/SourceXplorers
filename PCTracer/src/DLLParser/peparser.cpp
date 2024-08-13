#include "peparser.h"

namespace peparser
{
	using namespace std;

	PEParser::PEParser() :
		peBase_(nullptr), peStruct_({ 0, })
	{
		logger_.setLogType(LOG_LEVEL_ERROR, LOG_DIRECTION_CONSOLE, TRUE);
	};

	PEParser::~PEParser()
	{
		close();
	};

	void PEParser::close(void)
	{
		// ������ ��ü(PEFile or PEProcess) ����
		delete peBase_;
		peBase_ = nullptr;

		// peStruct_ ��� �ʱ�ȭ
		peStruct_.baseAddress = 0;
		peStruct_.imageBase = 0;
		peStruct_.sizeOfHeaders = 0;
		peStruct_.numberofSections = 0;
		peStruct_.dosHeader = nullptr;
		peStruct_.fileHader = nullptr;
		peStruct_.ntHeader32 = nullptr;
		peStruct_.sectionHeader = nullptr;
		peStruct_.dataDirectory = nullptr;
		peStruct_.sectionList.clear();
		peStruct_.exportFunctionList.clear();
		peStruct_.importFunctionList.clear();
	};

	bool PEParser::open(const tstring_view& filePath)
	{
		PEFile* peFile = new PEFile();
		peBase_ = reinterpret_cast<IPEBase*>(peFile);
		return peFile->open(filePath);
	};

	bool PEParser::parseEATCustom(const PE_DIRECTORY_TYPE& parseType)
	{
		if (peBase_->isPE())
		{
			// Set base address
			peStruct_.baseAddress = peBase_->getBaseAddress();

			// PE header parsing
			parseHeaders();

			// PE DataDirectory entry parsing
			if ((parseType & PE_DIRECTORY_EAT) == PE_DIRECTORY_EAT)
			{
				parseEAT();
			};

			return true;
		}
		else
		{
			return false;
		}
	};


	// PE ������ ���� ����ü ����
	const PE_STRUCT& PEParser::getPEStructure(void) const
	{
		return peStruct_;
	};

	// PE ������ ���ڿ��� UTF8 ���ڿ��̱� ������ ��ȯ �ʿ�
	tstring PEParser::getPEString(const char8_t* peStr)
	{
		return strConv_.to_tstring(peStr);
	};

	// ���� �̸� ���� ��� �ִ� 8����Ʈ�� �̿��ؼ� �̸��� ����Ǿ� �ִµ�
	// NULL ����('\0') ���� ��ü ����Ʈ ��� ���ڷ� �� ���� �� �ֱ� ������ 
	// �� �κ��� �����ؼ� ó�� �ʿ�
	tstring PEParser::getPEString(const char8_t* peStr, const size_t& maxLength)
	{
		// UTF8 ���ڿ��� ����(���⿡�� NULL ���ڰ� ���� �� ���Ե� ������ �� ����)
		const u8string u8string(peStr, maxLength);

		// c_str()�� ���ؼ� ���ʿ��� NULL ���ڸ� ������ ���·� ���ڿ� ��ȯ
		// ( c_str()�� NULL ���ڰ� ���Ե� ���ڿ��� ���� )
		return strConv_.to_tstring(u8string.c_str());
	};

	void PEParser::parseSectionHeaders(void)
	{
		size_t realAddress = 0;
		PIMAGE_SECTION_HEADER sectionHeader = peStruct_.sectionHeader;

		for (WORD index = 0; index < peStruct_.numberofSections; index++)
		{
			// ������ ���� �ּҸ� ����

			if (typeid(*peBase_) == typeid(PEFile))
			{
				// ������ ������ ��� RVA�� VirtualAddress�̱� ������ PointerToRawData�� RAW
				// RAW = RVA - VirtualAddress + PointerToRawData
				realAddress = peBase_->getBaseAddress() + sectionHeader->PointerToRawData;
			}

			// ������ �ֿ� ������ ���� SECTION_INFO ����ü�� sectionList_�� �߰�
			peStruct_.sectionList.push_back(
				SECTION_INFO{
					getPEString(reinterpret_cast<const char8_t*>(sectionHeader->Name),
					sizeof(sectionHeader->Name)),
					static_cast<size_t>(sectionHeader->VirtualAddress),
					static_cast<size_t>(sectionHeader->PointerToRawData),
					static_cast<size_t>(sectionHeader->SizeOfRawData),
					static_cast<size_t>(sectionHeader->Characteristics),
					realAddress
				}
			);

			// ���� ���� ����� ������ �̵�
			sectionHeader++;
		}

		// RVA to RAW ��� ���� ���ؼ� �ʿ��� ���� ������ ����
		peBase_->setSectionList(peStruct_.sectionList);
	};

	void PEParser::parseHeaders(void)
	{
		peStruct_.dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(peStruct_.baseAddress);
		peStruct_.is32Bit = peBase_->is32bit();

		if (peStruct_.is32Bit)
		{
			peStruct_.ntHeader32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew);
			peStruct_.fileHader = reinterpret_cast<PIMAGE_FILE_HEADER>(&(peStruct_.ntHeader32->FileHeader));
			peStruct_.sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
			peStruct_.imageBase = peStruct_.ntHeader32->OptionalHeader.ImageBase;
			peStruct_.dataDirectory = peStruct_.ntHeader32->OptionalHeader.DataDirectory;
			peStruct_.sizeOfHeaders = peStruct_.ntHeader32->OptionalHeader.SizeOfHeaders;
		}
		else
		{
			peStruct_.ntHeader64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew);
			peStruct_.fileHader = reinterpret_cast<PIMAGE_FILE_HEADER>(&(peStruct_.ntHeader64->FileHeader));
			peStruct_.sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(peStruct_.baseAddress + peStruct_.dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64));
			peStruct_.imageBase = peStruct_.ntHeader64->OptionalHeader.ImageBase;
			peStruct_.dataDirectory = peStruct_.ntHeader64->OptionalHeader.DataDirectory;
			peStruct_.sizeOfHeaders = peStruct_.ntHeader64->OptionalHeader.SizeOfHeaders;
		}
		peStruct_.numberofSections = peStruct_.fileHader->NumberOfSections;

		// RvaToRaw ��� ���� ���ؼ� ������� ��ü ũ��(PE Header + Section Header) ����
		peBase_->setHeaderSize(peStruct_.sizeOfHeaders);

		// Parse section header
		parseSectionHeaders();
	};

	void PEParser::parseEAT(void)
	{
		tstring moduleName;
		size_t exportDescriptorAddress = 0;
		size_t exportDescriptorRva = 0;
		size_t odinalBase = 0;
		size_t functionsAddress = 0;
		size_t nameOrdinalsAddress = 0;
		size_t namesAddress = 0;
		size_t realNamesAddress = 0;
		PIMAGE_EXPORT_DIRECTORY exportDirectory = nullptr;
		Functionist functionList;

		// EAT�� DataDirectory�� ù ��°
		exportDescriptorRva = peStruct_.dataDirectory[0].VirtualAddress;
		if (exportDescriptorRva != 0x0)
		{
			// PE ���Ͽ��� Export Directory(IMAGE_EXPORT_DIRECTORY)�� �ϳ��� ����
			if (peBase_->getRealAddress(exportDescriptorRva, exportDescriptorAddress))
			{
				exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(exportDescriptorAddress);

				// Export Module Name : �Լ��� �����ϴ� exe�� dll�� �̸�
				if (peBase_->getRealAddress(exportDirectory->Name, realNamesAddress))
				{
					moduleName = getPEString(reinterpret_cast<const char8_t*>(realNamesAddress));
				}
				else
				{
					moduleName.clear();
					logger_.log(format(_T("Can't get module name address : RVA = 0x{:x}"), static_cast<size_t>(exportDirectory->Name)), LOG_LEVEL_ERROR);
				}

				// Ordinal �� function address index + Base(odinalBase) ��
				odinalBase = exportDirectory->Base;

				// AddressOfFunctions : Export�ϴ� �Լ��� �ּҵ��� ��� �ִ� �迭�� ���� �ּ�
				// AddressOfNames : Export�ϴ� �Լ� �̸��� �ּҵ��� ��� �ִ� �迭�� ���� �ּ�
				// AddressOfNameOrdinals : �Լ� �̸����� Export�ϴ� �Լ����� �Լ� �ּҸ� ��� ���� �ʿ��� 
				//                         �Լ� �ּ� �迭������ ��ġ(index)���� ��� �ִ� �迭�� ���� �ּ�
				if ((peBase_->getRealAddress(exportDirectory->AddressOfFunctions, functionsAddress)) &&
					(peBase_->getRealAddress(exportDirectory->AddressOfNames, namesAddress)) &&
					(peBase_->getRealAddress(exportDirectory->AddressOfNameOrdinals, nameOrdinalsAddress)))
				{
					// �̸��� �����ϴ� �Լ��� üũ�� ���� ���� functionNameList ����
					vector<tstring> functionNameList(exportDirectory->NumberOfFunctions);

					// �̸��� �����ϴ� �Լ����� �̸��� functionNameList�� ����
					// �̸��� �������� �ʴ� �Լ����� �������� ����
					for (DWORD index = 0; index < exportDirectory->NumberOfNames; index++)
					{
						if (peBase_->getRealAddress(reinterpret_cast<DWORD*>(namesAddress)[index], realNamesAddress))
						{
							functionNameList[reinterpret_cast<WORD*>(nameOrdinalsAddress)[index]] 
								= getPEString(reinterpret_cast<char8_t*>(realNamesAddress));
						}
					}

					// ��ü �Լ����� ������ ��Ͽ� ����
					for (DWORD index = 0; index < exportDirectory->NumberOfFunctions; index++)
					{
						if (functionNameList[index].empty())
						{
							// Ordinal�θ� Export�ϴ� �Լ� �߰�
							if (reinterpret_cast<DWORD*>(functionsAddress)[index] != 0)
							{
								functionList.push_back(
									FUNCTION_INFO{
										tstring(_T("")),
										static_cast<size_t>(odinalBase + index),
										static_cast<size_t>(reinterpret_cast<DWORD*>(functionsAddress)[index])
									});
							}
							else
							{
								logger_.log(format(_T("Export address is invalid > 0x{:x}, 0x{:x}"), 
									reinterpret_cast<DWORD*>(functionsAddress)[index], (WORD)odinalBase + index), LOG_LEVEL_ERROR);
							}
						}
						else
						{
							// �̸��� �����ϴ� �Լ� �߰�
							functionList.push_back(
								FUNCTION_INFO{
									functionNameList[index],
									static_cast<size_t>(odinalBase + index),
									static_cast<size_t>(reinterpret_cast<DWORD*>(functionsAddress)[index])
								});
						}
					}
				}
				peStruct_.exportFunctionList.push_back(make_tuple(moduleName, functionList));
			}
		}
	};

};
