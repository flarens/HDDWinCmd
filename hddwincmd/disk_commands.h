#pragma once

#include "functions.h"

// winapi для атрибутов отключить/подключить диск
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


// проверить принадлежит ли логический том (по его букве) к указанному номеру диска
int CheckVolumeLetterOnDiskNumber(const wchar_t volumeLetter, const int deviceNumber, s_resp& resp);

// получить буквы всех дисков, содержащих файл подкачки
bool GetVolumeLettersContainingSwapFile(std::wstring& letters, s_resp& resp);

// проверка диска на наличие системных файлов
bool IsNotSystemDisk(const HANDLE hDevice, s_resp& resp);

// синхронизировать кэш диска
bool SendSynchronizeCache(const HANDLE hDevice);

// ожидание завершения операций с диском
bool WaitDiskActivity(const HANDLE hDevice, const DWORD timeout, s_resp& resp);

// остановить шпиндель
bool SendSpinDown(const HANDLE hDevice);

// запустить шпиндель
bool SendSpinUp(const HANDLE hDevice);

// усыпить диск
bool DiskSpinDown(const std::wstring& devicePath, const DWORD timeout, s_resp& resp);

// разбудить диск
bool DiskSpinUp(const std::wstring& devicePath, s_resp& resp);

// команда отключить/подключить диск
bool SendDiskAvailability(const HANDLE hDevice, const bool availability, const bool permanent, s_resp& resp);

// отключить диск от системы
bool DiskGoOffline(const std::wstring& devicePath, const bool permanent, const bool force, const DWORD timeout, s_resp& resp);

// подключить диск к системе
bool DiskGoOnline(const std::wstring& devicePath, const bool permanent, s_resp& resp);