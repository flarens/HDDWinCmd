#include <string>
#include <iostream>
#include <cctype>
#include <codecvt>

#include "functions.h"
#include "disk_info.h"


// ������� ���� ����� ���� response_type = text
void WcoutExt_Mini(const std::wstring& txt, s_resp& resp, const bool success) {
	if (resp.type > 1) std::wcout << txt;
	if (!success) resp.successAll = false;
}


// ������� ����� ��� response_type = 2, ������� success/failure ��� response_type = 1
void WcoutExt(const std::wstring& txt, s_resp& resp, const bool success) {
	if (resp.type == 1) {
		if (success) std::wcout << L"success" << std::endl;
		else std::wcout << L"failure" << std::endl;
	}
	if (resp.type == 2) std::wcout << txt;
	if (!success) resp.successAll = false;
}


// �������� - ������ ������ �� ����
bool IsDigitsOnly(const std::wstring& str) {
	// ������� �����
	if (str.size() == 0)
		return false;

	for (wchar_t ch : str) {
		if (!iswdigit(ch)) { // �������� �� ������ ������
			return false;
		}
	}
	return true;
}


// �������� - ������ ���� /dev/sda
bool IsLinuxDriveFormat(const std::wstring& str) {
	// ������� "/dev/sd"
	if (str.size() < 8)
		return false;

	// ���������� � "/dev/sd"
	const std::wstring prefix = L"/dev/sd";
	if (str.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}

	// ���������� ����� ������ ������� ������ �� ��������� ���� a-z
	for (size_t i = prefix.size(); i < str.size(); ++i) {
		if (!std::islower(str[i]) || str[i] < L'a' || str[i] > L'z') {
			return false;
		}
	}

	return true;
}


// �������� - ���� ��������� �����
bool IsOneLetter(const std::wstring& str) {
	// 1 ������
	if (str.size() != 1)
		return false;

	wchar_t ch = str[0];
	if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'))
		return true;

	return false;
}


// �������� - ������ ���� volume GUID
bool IsVolumeGUID(const std::wstring& str) {
	// ����� �� ������ ��� ��� "\\?\Volume{248f64f0-56e5-11ee-bfcd-00e04caa0bd0}\"
	if (str.size() < 48 || str.size() > 49)
		return false;

	// ���������� � "\\?\Volume{"
	const std::wstring prefix = L"\\\\?\\Volume{";
	if (str.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}

	// ������ �����������
	if (str[47] != L'}')
		return false;

	return true;
}


// ������ � ������������� ������
std::wstring StringToWstring(const char* str) {
	return std::wstring(str, str + std::strlen(str));
}

std::wstring StringToWstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}


// ������������� ����� � ��������� ������������� ���������� ������ Linux
std::wstring NumberTolinuxDriveLetters(int num) {
	std::wstring result = L"";
	while (num >= 0) {
		result = static_cast<wchar_t>(L'a' + (num % 26)) + result;
		num = num / 26 - 1;
	}
	return result;
}


// �������������� ������������������ ���� Linux ����� � �����
std::wstring LinuxDriveLettersToNumber(const std::wstring& input) {
	int result = 0;
	for (size_t i = 7; i < input.size(); ++i) {
		result = result * 26 + (input[i] - L'a' + 1);
	}
	return std::to_wstring(result - 1);
}


// �������������� GUID � ������
std::wstring GUIDtoString(const GUID& guid) {
	wchar_t guidString[39];
	if (StringFromGUID2(guid, guidString, sizeof(guidString) / sizeof(wchar_t)))
		return std::wstring(guidString);
	else
		return L"Failed to convert partition GUID to string";
}


// �������� ����� ����������� ����� �� volume GUID ������ �� ��� ���������� ������
int GetPhysicalDriveFromVolumeGUID(std::wstring volumePathName) {

	// ������ ���� � ����� ���� ����
	size_t len = volumePathName.size();
	if (len > 0 && volumePathName[len - 1] == L'\\') volumePathName[len - 1] = L'\0';

	// ������� ���
	HANDLE hVolume = CreateFile(volumePathName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return -1;
	}

	// ������� ����
	VOLUME_DISK_EXTENTS volumeDiskExtents;
	DWORD bytesReturned;
	if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &volumeDiskExtents, sizeof(volumeDiskExtents), &bytesReturned, NULL)) {
		CloseHandle(hVolume);
		return -1;
	}

	CloseHandle(hVolume);

	if (volumeDiskExtents.NumberOfDiskExtents > 0) {
		return volumeDiskExtents.Extents[0].DiskNumber;
	}

	return -1;
}


// ���������� ��������� drive
std::wstring GetPhysicalDriveFromDriveStr(const std::wstring& drive) {
	// �������� �����
	if (IsDigitsOnly(drive)) {
		return L"\\\\.\\PhysicalDrive" + drive;
	}
	// linux ������
	else if (IsLinuxDriveFormat(drive)) {
		return L"\\\\.\\PhysicalDrive" + LinuxDriveLettersToNumber(drive);
	}
	// ����� ����
	else if (IsOneLetter(drive)) {
		int deviceNumber;
		deviceNumber = GetPhysicalDriveFromVolumeGUID(GetVolumeGUIDFromDriveLetter(drive));
		if (deviceNumber > -1) return L"\\\\.\\PhysicalDrive" + std::to_wstring(deviceNumber);
		else return L"";
	}
	// volume GUID
	else if (IsVolumeGUID(drive)) {
		int deviceNumber;
		deviceNumber = GetPhysicalDriveFromVolumeGUID(drive);
		if (deviceNumber > -1) return L"\\\\.\\PhysicalDrive" + std::to_wstring(deviceNumber);
		else return L"";
	}

	return drive;
}


// ��������� ������ �����
int GetDriveNumber(const HANDLE& hDrive) {
	STORAGE_DEVICE_NUMBER deviceNumber;
	DWORD bytesReturned = 0;

	if (DeviceIoControl(
		hDrive,
		IOCTL_STORAGE_GET_DEVICE_NUMBER,
		NULL,
		0,
		&deviceNumber,
		sizeof(deviceNumber),
		&bytesReturned,
		NULL
	)) {
		return deviceNumber.DeviceNumber;
	}else {
		return -1;
	}
}


// �������������� ���-�� ���� � ������������� ���
std::wstring BytesToFormatString(long long bytes) {
	const wchar_t* suffixes[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB" };
	int suffixIndex = 0;

	double size = static_cast<double>(bytes);

	while (size >= 1024 && suffixIndex < 6) {
		size /= 1024;
		suffixIndex++;
	}

	wchar_t buffer[32];
	swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%.2f %ls", size, suffixes[suffixIndex]);

	return std::wstring(buffer);
}


// ��� ��������� ������ � ������������
std::wstring GetLastErrorString() {
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),		// MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) - ��� ����� ������� (������� ��� �������)
		buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

	// �������� �������� ������, ���� �� ����
	size_t len = wcslen(buf);
	if (len > 0 && buf[len - 1] == L'\n') {
		buf[len - 1] = L'\0';
	}
	if (len > 1 && buf[len - 2] == L'\r') {
		buf[len - 2] = L'\0';
	}

	return L"Error code: " + std::to_wstring(GetLastError()) + L" (" + buf + L")";
}


// ������� ����
bool OpenDevice(HANDLE& hDevice, const std::wstring& devicePath, s_resp& resp) {
	hDevice = CreateFileW(
		devicePath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE) {
		WcoutExt(L"Failed to open device [" + devicePath + L"]. " + GetLastErrorString() + L"\r\n", resp, false);
		return false;
	}

	return true;
}