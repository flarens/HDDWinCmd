#include <windows.h>
#include <string>
#include <ntddscsi.h>

#include "disk_commands.h"
#include "functions.h"

#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")


// проверить принадлежит ли логический том (по его букве) к указанному номеру диска
int CheckVolumeLetterOnDiskNumber(const wchar_t volumeLetter, const int deviceNumber, s_resp& resp) {
	// открываем логический том
	std::wstring volumePath = L"\\\\.\\";
	volumePath += volumeLetter;
	volumePath += L":";
	HANDLE hVolume = CreateFileW(volumePath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hVolume == INVALID_HANDLE_VALUE) {
		WcoutExt(L"Failed to open disk [" + std::wstring(1, volumeLetter) + L"]. " + GetLastErrorString() + L"\r\n", resp, false);
		return -1;
	}

	// запрос информации
	DWORD bytesReturned;
	VOLUME_DISK_EXTENTS diskExtents;
	if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, &diskExtents, sizeof(diskExtents), &bytesReturned, nullptr)) {
		WcoutExt(L"Failed to get disk [" + std::wstring(1, volumeLetter) + L"] information. " + GetLastErrorString() + L"\r\n", resp, false);
		CloseHandle(hVolume);
		return -1;
	}

	// перебираем все физические диски, на которых находится логический том
	for (DWORD i = 0; i < diskExtents.NumberOfDiskExtents; ++i) {
		DISK_EXTENT& extent = diskExtents.Extents[i];
		// номер выбранного диска совпадает с системным - он системный
		if (extent.DiskNumber == deviceNumber) {
			CloseHandle(hVolume);
			return 1;
		}
	}

	CloseHandle(hVolume);
	return 0;
}


// получить буквы всех дисков, содержащих файл подкачки
bool GetVolumeLettersContainingSwapFile(std::wstring& letters, s_resp& resp) {
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		WcoutExt(L"Failed to open registry key. " + GetLastErrorString() + L"\r\n", resp, false);
		return false;
	}

	DWORD size = 0;
	if (RegQueryValueExW(hKey, L"ExistingPageFiles", nullptr, nullptr, nullptr, &size) != ERROR_SUCCESS) {
		WcoutExt(L"Failed to request value from the registry. " + GetLastErrorString() + L"\r\n", resp, false);
		RegCloseKey(hKey);
		return false;
	}

	wchar_t* buffer = new wchar_t[size / sizeof(wchar_t)];
	if (RegQueryValueExW(hKey, L"ExistingPageFiles", nullptr, nullptr, (LPBYTE)buffer, &size) != ERROR_SUCCESS) {
		WcoutExt(L"Failed to request value from the registry. " + GetLastErrorString() + L"\r\n", resp, false);
		delete[] buffer;
		RegCloseKey(hKey);
		return false;
	}

	wchar_t* current = buffer;
	letters = L"";
	while (*current) {
		// путь вида \??\C:\pagefile.sys
		if (current[0] == L'\\' && current[1] == L'?' && current[2] == L'?' && current[3] == L'\\' && current[5] == L':') {
			letters += current[4];
		}
		current += wcslen(current) + 1;
	}

	delete[] buffer;
	RegCloseKey(hKey);
	return true;
}


// проверка диска на наличие системных файлов
bool IsNotSystemDisk(const HANDLE hDevice, s_resp& resp) {

	// получаем номер выбранного диска
	int driveNumber = GetDriveNumber(hDevice);
	if (driveNumber < 0) {
		WcoutExt(L"Failed to get disk number. " + GetLastErrorString() + L"\r\n", resp, false);
		return false;
	}

	// получаем системную директорию
	wchar_t systemPath[MAX_PATH];
	if (!GetWindowsDirectoryW(systemPath, MAX_PATH)) {
		WcoutExt(L"Failed to get system directory. " + GetLastErrorString() + L"\r\n", resp, false);
		return false;
	}

	// извлекаем букву системного тома
	wchar_t systemVolumeLetter = systemPath[0];

	// проверка
	int result = CheckVolumeLetterOnDiskNumber(systemVolumeLetter, driveNumber, resp);
	if (result < 0) {
		return false;
	}else if (result > 0) {
		WcoutExt(L"The selected disk contains the volume [" + std::wstring(1, systemVolumeLetter) + L"] with the current operating system, so it cannot be taken offline\r\n", resp, false);
		return false;
	}

	// получаем тома, содержащие файл подкачки
	std::wstring volumeLetters;
	if (!GetVolumeLettersContainingSwapFile(volumeLetters, resp)) {
		return false;
	}

	// проверяем все
	size_t len = volumeLetters.size();
	if (len > 0) {
		for (size_t i = 0; i < len; ++i) {
			result = CheckVolumeLetterOnDiskNumber(volumeLetters[i], driveNumber, resp);
			if (result < 0) {
				return false;
			}
			else if (result > 0) {
				WcoutExt(L"The selected disk contains the volume [" + std::wstring(1, volumeLetters[i]) + L"] where the swap file is located, so it cannot be taken offline\r\n", resp, false);
				return false;
			}
		}
	}

	return true;
}


// синхронизировать кэш диска
bool SendSynchronizeCache(const HANDLE hDevice) {
	SCSI_PASS_THROUGH spt = { 0 };
	spt.Length = sizeof(SCSI_PASS_THROUGH);
	spt.CdbLength = 10; // SYNCHRONIZE CACHE использует 10-byte CDB
	spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	spt.TimeOutValue = 30; // таймаут для синхронизации в секундах
	spt.Cdb[0] = 0x35; // SYNCHRONIZE CACHE команда

	DWORD returned = 0;

	if (!DeviceIoControl(
		hDevice,
		IOCTL_SCSI_PASS_THROUGH,
		&spt,
		sizeof(SCSI_PASS_THROUGH),
		NULL,
		0,
		&returned,
		NULL))
	{
		return false;
	}
	return true;
}


// ожидание завершения операций с диском
bool WaitDiskActivity(const HANDLE hDevice, const DWORD timeout, s_resp& resp) {
	DISK_PERFORMANCE diskPerformance = { 0 };
	DWORD bytesReturned = 0;
	DWORD ReadCountOld = 0;
	DWORD WriteCountOld = 0;
	int iter = 0;
	int iter_dif = 0;
	int iter_max = round(timeout / 1000) + 2;
	bool busy_flag = false;

	while (true) {
		if (DeviceIoControl(
			hDevice,
			IOCTL_DISK_PERFORMANCE,
			NULL,
			0,
			&diskPerformance,
			sizeof(diskPerformance),
			&bytesReturned,
			NULL)) {

			// номер итерации, на которой последний раз обнаружена активность
			if (diskPerformance.ReadCount != ReadCountOld || diskPerformance.WriteCount != WriteCountOld)
				iter_dif = iter;

			// сообщение о занятости диска операциями
			if (iter_dif > 0 && !busy_flag) {
				busy_flag = true;
				WcoutExt_Mini(L"Disk is busy, wait for end, but no more than " + std::to_wstring(timeout) + L" milliseconds...\r\n", resp, true);
			}

			// операций с диском не зафиксировано за 2 тика (секунды)
			if (iter - iter_dif >= 2) {
				if (busy_flag) WcoutExt_Mini(L"Successfully wait for disk operations to end\r\n", resp, true);
				return true;
			}

			// выход по таймауту
			if (iter >= iter_max) {
				WcoutExt(L"Error: disk waiting timeout expired\r\n", resp, false);
				return false;
			}

			ReadCountOld = diskPerformance.ReadCount;
			WriteCountOld = diskPerformance.WriteCount;
		}
		else {
			WcoutExt(L"Failed to retrieve disk performance data. " + GetLastErrorString() + L"\r\n", resp, false);
			return false;
		}

		Sleep(1000);
		iter++;
	}
}


// остановить шпиндель
bool SendSpinDown(const HANDLE hDevice) {
	SCSI_PASS_THROUGH spt = { 0 };
	spt.Length = sizeof(SCSI_PASS_THROUGH);
	spt.CdbLength = 6; // длина команды SCSI CDB
	spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
	spt.TimeOutValue = 2;	// таймаут в секундах
	spt.Cdb[0] = 0x1B;		// команда START STOP UNIT
	spt.Cdb[1] = 0;			// Logical Unit Number (LUN)
	spt.Cdb[4] = 0;			// остановить диск (SPINDOWN)
	spt.Cdb[5] = 0;			// control byte

	DWORD error;
	DWORD returned;
	int steps = 10;	// максимальное количество попыток

	// повторять если диск не успел уснуть за отведенный таймаут
	do {
		error = 0;
		returned = 0;
		steps--;
		// отправляем команду SCSI
		if (!DeviceIoControl(
			hDevice,
			IOCTL_SCSI_PASS_THROUGH,
			&spt,
			sizeof(SCSI_PASS_THROUGH),
			NULL,
			0,
			&returned,
			NULL))
		{
			error = GetLastError();
			if (error > 0 && error != ERROR_IO_DEVICE) {
				return false;
			}
		}else {
			return true;
		}

	} while (error == ERROR_IO_DEVICE && steps > 0);	// если код ошибки ERROR_IO_DEVICE (1117) - возможно устройство не успело уснуть

	return false;
}


// запустить шпиндель
bool SendSpinUp(const HANDLE hDevice) {
	DWORD bytesReturned;
	if (!DeviceIoControl(
		hDevice,
		IOCTL_DISK_UPDATE_PROPERTIES,	//IOCTL_DISK_CHECK_VERIFY,
		NULL,
		0,
		NULL,
		0,
		&bytesReturned,
		NULL
	)) {
		return false;
	}

	return true;
}


// усыпить диск
bool DiskSpinDown(const std::wstring& devicePath, const DWORD timeout, s_resp& resp) {
	// открываем диск
	HANDLE hDevice;
	if (!OpenDevice(hDevice, devicePath, resp)) {
		return false;
	}

	// синхронизируем кэш
	WcoutExt_Mini(L"Send command to synchronize disk cache...\r\n", resp, true);
	if (!SendSynchronizeCache(hDevice)) {
		WcoutExt(L"Failed to synchronize the disk cache before stopping the spindle. " + GetLastErrorString() + L"\r\n", resp, false);
		CloseHandle(hDevice);
		return false;
	}

	// проверка на операции чтения/записи
	WcoutExt_Mini(L"Checking disk activity...\r\n", resp, true);
	if (!WaitDiskActivity(hDevice, timeout, resp)) {
		CloseHandle(hDevice);
		return false;
	}

	// остановим
	WcoutExt_Mini(L"Send command to stop spindle...\r\n", resp, true);
	if (!SendSpinDown(hDevice)) {
		WcoutExt(L"Failed to stop disk spindle. " + GetLastErrorString() + L"\r\n", resp, false);
		CloseHandle(hDevice);
		return false;
	}

	WcoutExt(L"Spindle stop command successfully sent\r\n", resp, true);

	CloseHandle(hDevice);
	return true;
}


// разбудить диск
bool DiskSpinUp(const std::wstring& devicePath, s_resp& resp) {
	// открываем диск
	HANDLE hDevice;
	if (!OpenDevice(hDevice, devicePath, resp)) {
		return false;
	}

	// разбудим
	WcoutExt_Mini(L"Send command to wake-up disk...\r\n", resp, true);
	if (!SendSpinUp(hDevice)) {
		WcoutExt(L"Failed to start disk spindle. " + GetLastErrorString() + L"\r\n", resp, false);
		CloseHandle(hDevice);
		return false;
	}

	WcoutExt(L"Disk wakeup command successfully sent\r\n", resp, true);

	CloseHandle(hDevice);
	return true;
}


// команда отключить/подключить диск
bool SendDiskAvailability(const HANDLE hDevice, const bool availability, const bool permanent, s_resp& resp) {
	DWORD bytesReturned = 0;
	SET_DISK_ATTRIBUTES disk_attr;
	ZeroMemory(&disk_attr, sizeof(disk_attr));

	disk_attr.Version = sizeof(SET_DISK_ATTRIBUTES);
	disk_attr.Persist = permanent; // сохранить изменения после перезагрузки
	disk_attr.Attributes = !availability ? DISK_ATTRIBUTE_OFFLINE : 0;
	disk_attr.AttributesMask = DISK_ATTRIBUTE_OFFLINE;

	// применить атрибуты доступности диска
	if (!DeviceIoControl(hDevice, IOCTL_DISK_SET_DISK_ATTRIBUTES, &disk_attr, disk_attr.Version, NULL, 0, &bytesReturned, NULL)) {
		WcoutExt(L"Failed to set disk availability attributes. " + GetLastErrorString() + L"\r\n", resp, false);
		return false;
	}

	// обновить перечисление устройств, если диск подключили к системе
	if (availability) {
		if (!DeviceIoControl(hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
			WcoutExt(L"Failed to update disk properties after applying its accessibility to the online state. " + GetLastErrorString() + L"\r\n", resp, false);
			return false;
		}
	}

	return true;
}


// отключить диск от системы
bool DiskGoOffline(const std::wstring& devicePath, const bool permanent, const bool force, const DWORD timeout, s_resp& resp) {
	// открываем диск
	HANDLE hDevice;
	if (!OpenDevice(hDevice, devicePath, resp)) {
		return false;
	}
	
	// проверка диска на наличие системных файлов
	WcoutExt_Mini(L"Checking the selected disk...\r\n", resp, true);
	if (!IsNotSystemDisk(hDevice, resp)) {
		CloseHandle(hDevice);
		return false;
	}

	// проверка на операции чтения/записи
	if (!force) {
		WcoutExt_Mini(L"Checking disk activity...\r\n", resp, true);
		if (!WaitDiskActivity(hDevice, timeout, resp)) {
			CloseHandle(hDevice);
			return false;
		}
	}

	// отключим диск
	WcoutExt_Mini(L"Applying offline mode to the selected disk...\r\n", resp, true);
	if (!SendDiskAvailability(hDevice, false, permanent, resp)) {
		CloseHandle(hDevice);
		return false;
	}

	WcoutExt(L"Disk has been successfully offline\r\n", resp, true);

	CloseHandle(hDevice);
	return true;
}


// подключить диск к системе
bool DiskGoOnline(const std::wstring& devicePath, const bool permanent, s_resp& resp) {

	// открываем диск
	HANDLE hDevice;
	if (!OpenDevice(hDevice, devicePath, resp)) {
		return false;
	}

	// подключим диск
	WcoutExt_Mini(L"Applying online mode to the selected disk...\r\n", resp, true);
	if (!SendDiskAvailability(hDevice, true, permanent, resp)) {
		CloseHandle(hDevice);
		return false;
	}

	WcoutExt(L"Disk has been successfully online\r\n", resp, true);

	CloseHandle(hDevice);
	return true;
}