#pragma once

#include <windows.h>
#include <string>

// ��������������� ��������� ��� ����� � �������
struct s_resp {
	int type;
	bool successAll;
};

// ������� ���� ����� ���� response_type = text
void WcoutExt_Mini(const std::wstring& txt, s_resp& resp, const bool success);
// ������� ����� ��� response_type = 2, ������� success/failure ��� response_type = 1
void WcoutExt(const std::wstring& txt, s_resp& resp, const bool success);

// �������� - ������ ������ �� ����
bool IsDigitsOnly(const std::wstring& str);

// �������� - ������ ���� /dev/sda
bool IsLinuxDriveFormat(const std::wstring& str);

// �������� - ���� ��������� �����
bool IsOneLetter(const std::wstring& str);

// �������� - ������ ���� volume GUID
bool IsVolumeGUID(const std::wstring& str);

// ������ � ������������� ������
std::wstring StringToWstring(const char* str);
std::wstring StringToWstring(const std::string& str);

// ������������� ����� � ��������� ������������� ���������� ������ Linux
std::wstring NumberTolinuxDriveLetters(int num);

// �������������� ������������������ ���� Linux ����� � �����
std::wstring LinuxDriveLettersToNumber(const std::wstring& input);

// �������������� GUID � ������
std::wstring GUIDtoString(const GUID& guid);

// �������� ����� ����������� ����� �� volume GUID ������ �� ��� ���������� ������
int GetPhysicalDriveFromVolumeGUID(std::wstring volumePathName);

// ���������� ��������� drive
std::wstring GetPhysicalDriveFromDriveStr(const std::wstring& drive);

// ��������� ������ �����	
int GetDriveNumber(const HANDLE& hDrive);

// �������������� ���-�� ���� � ������������� ���
std::wstring BytesToFormatString(long long bytes);

// ��� ��������� ������ � ������������
std::wstring GetLastErrorString();

// ������� ����
bool OpenDevice(HANDLE& hDevice, const std::wstring& devicePath, s_resp& resp);