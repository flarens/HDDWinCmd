#pragma once

#include "functions.h"

// winapi ��� ��������� ���������/���������� ����
typedef struct _SET_DISK_ATTRIBUTES {
	DWORD Version;
	BOOLEAN Persist;
	BYTE  Reserved1[3];
	DWORDLONG Attributes;
	DWORDLONG AttributesMask;
	DWORD Reserved2[4];
} SET_DISK_ATTRIBUTES, *PSET_DISK_ATTRIBUTES;
#define DISK_ATTRIBUTE_OFFLINE              0x0000000000000001
#define IOCTL_DISK_SET_DISK_ATTRIBUTES      CTL_CODE(IOCTL_DISK_BASE, 0x003d, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


// ��������� ����������� �� ���������� ��� (�� ��� �����) � ���������� ������ �����
int CheckVolumeLetterOnDiskNumber(const wchar_t volumeLetter, const int deviceNumber, s_resp& resp);

// �������� ����� ���� ������, ���������� ���� ��������
bool GetVolumeLettersContainingSwapFile(std::wstring& letters, s_resp& resp);

// �������� ����� �� ������� ��������� ������
bool IsNotSystemDisk(const HANDLE hDevice, s_resp& resp);

// ���������������� ��� �����
bool SendSynchronizeCache(const HANDLE hDevice);

// �������� ���������� �������� � ������
bool WaitDiskActivity(const HANDLE hDevice, const DWORD timeout, s_resp& resp);

// ���������� ��������
bool SendSpinDown(const HANDLE hDevice);

// ��������� ��������
bool SendSpinUp(const HANDLE hDevice);

// ������� ����
bool DiskSpinDown(const std::wstring& devicePath, const DWORD timeout, s_resp& resp);

// ��������� ����
bool DiskSpinUp(const std::wstring& devicePath, s_resp& resp);

// ������� ���������/���������� ����
bool SendDiskAvailability(const HANDLE hDevice, const bool availability, const bool permanent, s_resp& resp);

// ��������� ���� �� �������
bool DiskGoOffline(const std::wstring& devicePath, const bool permanent, const bool force, const DWORD timeout, s_resp& resp);

// ���������� ���� � �������
bool DiskGoOnline(const std::wstring& devicePath, const bool permanent, s_resp& resp);