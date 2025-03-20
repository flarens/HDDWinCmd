#include <string>
#include <iostream>
#include <cctype>
#include <codecvt>
#include <algorithm>
#include <io.h>

#include "functions.h"
#include "disk_info.h"


// выводит весь текст если response_type = text
void WcoutExt_Mini(const std::wstring& txt, s_resp& resp, const bool success) {
	resp.logger.Log(txt);

	if (resp.type > 1) std::wcout << txt;
	if (!success) resp.successAll = false;
}


// выводит текст при response_type = 2, выводит success/failure при response_type = 1
void WcoutExt(const std::wstring& txt, s_resp& resp, const bool success) {
	resp.logger.Log(txt);

	if (resp.type == 1) {
		if (success) std::wcout << L"success" << std::endl;
		else std::wcout << L"failure" << std::endl;
	}
	if (resp.type == 2) std::wcout << txt;
	if (!success) resp.successAll = false;
}


// проверка - строка только из цифр
bool IsDigitsOnly(const std::wstring& str) {
	// нулевая длина
	if (str.size() == 0)
		return false;

	for (wchar_t ch : str) {
		if (!iswdigit(ch)) { // является ли символ цифрой
			return false;
		}
	}
	return true;
}


// проверка - строка вида /dev/sda
bool IsLinuxDriveFormat(const std::wstring& str) {
	// длиннее "/dev/sd"
	if (str.size() < 8)
		return false;

	// начинается с "/dev/sd"
	const std::wstring prefix = L"/dev/sd";
	if (str.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}

	// оставшаяся часть строки состоит только из латинских букв a-z
	for (size_t i = prefix.size(); i < str.size(); ++i) {
		if (!std::islower(str[i]) || str[i] < L'a' || str[i] > L'z') {
			return false;
		}
	}

	return true;
}


// проверка - путь к диску вида \\.\PhysicalDriveX
bool IsPhysicalDrivePath(const std::wstring& str) {
	// префикс
	const std::wstring prefix = L"\\\\.\\PhysicalDrive";
	if (str.compare(0, prefix.length(), prefix) != 0) {
		return false;
	}

	// после префикса идут только цифры и их не менее одной
	size_t pos = prefix.length();
	if (pos >= str.length()) {
		return false;
	}

	for (; pos < str.length(); ++pos) {
		wchar_t ch = str[pos];
		if (ch < L'0' || ch > L'9') {
			return false; // нецифровой символ
		}
	}

	return true;
}


// проверка - латинская буква
bool IsLatinLetter(const wchar_t ch) {
	if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z'))
		return true;
	else
		return false;
}


// проверка - одна латинская буква
bool IsOneLetter(const std::wstring& str) {
	// 1 символ
	if (str.size() != 1)
		return false;

	if (IsLatinLetter(str[0]))
		return true;

	return false;
}


// проверка - строка вида volume GUID
bool IsVolumeGUID(const std::wstring& str) {
	// длина со слешем или без "\\?\Volume{248f64f0-56e5-11ee-bfcd-00e04caa0bd0}\"
	if (str.size() < 48 || str.size() > 49)
		return false;

	// начинается с "\\?\Volume{"
	const std::wstring prefix = L"\\\\?\\Volume{";
	if (str.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}

	// скобка закрывающая
	if (str[47] != L'}')
		return false;

	return true;
}


// проверка - строка вида DevicePath
bool IsDevicePath(const std::wstring& str) {
	const std::wstring prefix = L"\\\\?\\";
	const std::wstring suffix = L"#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}";

	// не короче prefix + suffix
	if (str.length() < prefix.length() + suffix.length()) {
		return false;
	}

	// начинается с prefix
	if (str.compare(0, prefix.size(), prefix) != 0) {
		return false;
	}

	// заканчивается на suffix
	if (str.compare(str.length() - suffix.length(), suffix.length(), suffix) != 0) {
		return false;
	}

	return true;
}


// проверка - это файловый путь на диске
bool IsPath(const std::wstring& str) {
	// путь в формате "C:\folder..."
	if (str.size() > 3
		&& IsLatinLetter(str[0])
		&& str[1] == L':'
		&& str[2] == L'\\') {
		return true;
	}

	// путь в формате "\\?\C:\folder..."
	else if (str.size() > 7
		&& str[0] == L'\\'
		&& str[1] == L'\\'
		&& str[2] == L'?'
		&& str[3] == L'\\'
		&& IsLatinLetter(str[4])
		&& str[5] == L':'
		&& str[6] == L'\\') {
		return true;
	}

	return false;
}


// проверка - это корневой путь на диске, заданный буквой
bool IsLetterRootPath(const std::wstring& str) {
	// путь в формате "C:\"
	if (str.size() == 3
		&& IsLatinLetter(str[0])
		&& str[1] == L':'
		&& str[2] == L'\\') {
		return true;
	}

	// путь в формате "\\?\C:\"
	else if (str.size() == 7
		&& str[0] == L'\\'
		&& str[1] == L'\\'
		&& str[2] == L'?'
		&& str[3] == L'\\'
		&& IsLatinLetter(str[4])
		&& str[5] == L':'
		&& str[6] == L'\\') {
		return true;
	}

	return false;
}


// является ли путь корнем диска
int IsRootPath(std::wstring path) {
	// удалим завершающий слеш, если он есть
	if (path.back() == L'\\') {
		path.pop_back();
	}

	// путь в формате "C:\" или "\\?\C:\"
	size_t posPoints = path.find(L':');
	if (posPoints != std::wstring::npos) {
		size_t posSlash = path.find(L'\\', posPoints + 1); // ищем первый слеш после диска
		if (posSlash == std::wstring::npos) {
			return 1;
		}
	}

	// путь в формате GUID тома
	else if (path.find(L"\\\\?\\Volume{") == 0) {
		size_t posSlash = path.find(L'\\', 48); // ищем первый слеш после GUID
		if (posSlash == std::wstring::npos) {
			return 1;
		}
	}

	// некорректный путь
	else return -1;

	return 0;
}


// строка в многобайтовую строку
std::wstring StringToWstring(const char* str) {
	return std::wstring(str, str + std::strlen(str));
}

std::wstring StringToWstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}


// преобразовать число в буквенное представление изчисления дисков Linux
std::wstring NumberTolinuxDriveLetters(int num) {
	std::wstring result = L"";
	while (num >= 0) {
		result = static_cast<wchar_t>(L'a' + (num % 26)) + result;
		num = num / 26 - 1;
	}
	return result;
}


// преобразование последовательности букв Linux диска в число
std::wstring LinuxDriveLettersToNumber(const std::wstring& input) {
	int result = 0;
	for (size_t i = 7; i < input.size(); ++i) {
		result = result * 26 + (input[i] - L'a' + 1);
	}
	return std::to_wstring(result - 1);
}


// конвертировать GUID в строку
std::wstring GUIDtoString(const GUID& guid) {
	wchar_t guidString[39];
	if (StringFromGUID2(guid, guidString, sizeof(guidString) / sizeof(wchar_t)))
		return std::wstring(guidString);
	else
		return L"Failed to convert partition GUID to string";
}


// получить номер физического диска по volume GUID одного из его логических дисков
int GetPhysicalDriveFromVolumeGUID(std::wstring volumePathName) {

	// уберем слеш с конца если есть
	size_t len = volumePathName.size();
	if (len > 0 && volumePathName[len - 1] == L'\\') volumePathName[len - 1] = L'\0';

	// откроем том
	HANDLE hVolume = CreateFile(volumePathName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return -1;
	}

	// получим инфу
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


// перейли к источнику символической ссылки
std::wstring GetSymbolicLinkTarget(const std::wstring& symlinkPath) {
	// открываем симлинк
	HANDLE hFile = CreateFileW(
		symlinkPath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
	);

	// ошибка открытия симлинка
	if (hFile == INVALID_HANDLE_VALUE) {
		return L"";
	}

	// получаем конечный путь
	wchar_t buffer[MAX_PATH];
	DWORD result = GetFinalPathNameByHandleW(
		hFile,
		buffer,
		MAX_PATH,
		FILE_NAME_NORMALIZED | VOLUME_NAME_GUID
	);

	CloseHandle(hFile);

	// ошибка получения конечного пути
	if (result == 0) {
		return L"";
	}

	return std::wstring(buffer, result);
}


// получить родительскую директорию
std::wstring GetParentFolder(std::wstring path) {
	// удалим завершающий слеш, если он есть
	if (path.back() == L'\\') {
		path.pop_back();
	}

	// удалим все от последнего слеша
	size_t lastSlash = path.find_last_of(L"\\");
	if (lastSlash != std::wstring::npos) {
		path = path.substr(0, lastSlash);
	}else {
		path.clear();
	}

	return path;
}


// трассировка пути к корню диска с учетом симлинков и точек монтирования
std::wstring TraseToRoot(const std::wstring& path) {
	std::wstring currentPath = path;

	// трассировка
	while (!currentPath.empty()) {

		// проверка пути на корень диска
		int is_root = IsRootPath(currentPath);

		// некорректный путь
		if (is_root == -1)
			return path;

		// добавляем завершающий слеш, если его нет
		if (currentPath.back() != L'\\') {
			currentPath += L'\\';
		}

		// это корень диска
		if (is_root == 1)
			return currentPath;

		DWORD attributes = GetFileAttributes(currentPath.c_str());

		if (attributes == INVALID_FILE_ATTRIBUTES) {
			return path;
		}

		if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
			// проверка на символическую ссылку
			if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
				
				wchar_t volumePath[MAX_PATH];
				if (!GetVolumePathNameW(currentPath.c_str(), volumePath, MAX_PATH)) {
					return path;
				}

				if (currentPath == volumePath) {
					// обработка точки монтирования
					wchar_t volumeName[MAX_PATH];
					if (GetVolumeNameForVolumeMountPointW(currentPath.c_str(), volumeName, MAX_PATH)) {
						return std::wstring(volumeName);
					}else {
						return path;
					}

				}else {
					// обработка симлинка
					currentPath = GetSymbolicLinkTarget(currentPath);
				}
			}
		}

		// проверка пути на корень диска
		is_root = IsRootPath(currentPath);

		// некорректный путь
		if (is_root == -1)
			return path;

		// это корень диска
		if (is_root == 1) {
			// добавляем завершающий слеш, если его нет
			if (currentPath.back() != L'\\') {
				currentPath += L'\\';
			}
			return currentPath;
		}
	
		// родительский каталог
		currentPath = GetParentFolder(currentPath);
	}
}


// распознать введенный drive
std::wstring GetPhysicalDriveFromDriveStr(std::wstring drive) {

	// трассировка пути
	if (IsPath(drive)) {
		drive = TraseToRoot(drive);
	}

	// обрамление буквы тома
	if (IsOneLetter(drive)) {
		drive = drive + L":\\";
	}

	// передано число
	if (IsDigitsOnly(drive)) {
		return L"\\\\.\\PhysicalDrive" + drive;
	}
	// linux формат
	else if (IsLinuxDriveFormat(drive)) {
		return L"\\\\.\\PhysicalDrive" + LinuxDriveLettersToNumber(drive);
	}
	// логический диск буквой
	else if (IsLetterRootPath(drive)) {
		int deviceNumber;
		deviceNumber = GetPhysicalDriveFromVolumeGUID(GetVolumeGUIDFromDriveLetter(drive));
		if (deviceNumber > -1) return L"\\\\.\\PhysicalDrive" + std::to_wstring(deviceNumber);
		else return L"";
	}
	// VolumeGUID
	else if (IsVolumeGUID(drive)) {
		int deviceNumber;
		deviceNumber = GetPhysicalDriveFromVolumeGUID(drive);
		if (deviceNumber > -1) return L"\\\\.\\PhysicalDrive" + std::to_wstring(deviceNumber);
		else return L"";
	}
	// DevicePath
	else if (IsDevicePath(drive)) {
		return drive;
	}
	// вида \\.\PhysicalDriveX
	else if (IsPhysicalDrivePath(drive)) {
		return drive;
	}

	// осталось либо DeviceID, либо что-то невалидное
	return ConvertDriveInstanceIdToDevicePath(drive);
}


// получение номера диска
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


// конвертировать кол-во байт в удобочитаемый вид
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


// код последней ошибки с расшифровкой
std::wstring GetLastErrorString() {
	wchar_t buf[256];
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),		// MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) - для языка системы (русский для русских)
		buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

	// удаление перевода строки, если он есть
	size_t len = wcslen(buf);
	if (len > 0 && buf[len - 1] == L'\n') {
		buf[len - 1] = L'\0';
	}
	if (len > 1 && buf[len - 2] == L'\r') {
		buf[len - 2] = L'\0';
	}

	return L"Error code: " + std::to_wstring(GetLastError()) + L" (" + buf + L")";
}


// открыть диск
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


// преобразование DevicePath в DeviceInstanceId
std::wstring ConvertDevicePathToDeviceInstanceId(const std::wstring& devicePath) {
	std::wstring result = devicePath;

	// длина строки
	if (result.size() < 4) {
		return L"";
	}

	// отбрасываем префикс "\\?\"
	if (result.substr(0, 4) == L"\\\\?\\") {
		result = result.substr(4);
	}
	else {
		return L"";
	}

	// удаляем суффикс "{...}"
	size_t lastRBracePos = result.find_last_of(L'}');
	size_t lastLBracePos = result.find_last_of(L'{');
	if (lastRBracePos != std::wstring::npos && lastLBracePos != std::wstring::npos && lastRBracePos == result.size() - 1 && lastLBracePos < lastRBracePos) {
		result = result.substr(0, lastLBracePos);
	}
	else {
		return L"";
	}

	// удаляем завершающий символ "#"
	if (!result.empty() && result.back() == L'#') {
		result.pop_back();
	}

	// заменяем все символы "#" на "\"
	std::replace(result.begin(), result.end(), L'#', L'\\');

	// приводим все буквы к верхнему регистру.
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);

	// проверка на запрещенные символы
	const std::wstring invalidChars = L"/*:?\"<>|";
	for (wchar_t ch : result) {
		if (invalidChars.find(ch) != std::wstring::npos || ch < 32) {
			return L"";
		}
	}

	return result;
}


// преобразование DeviceInstanceId дискового устройства в DevicePath
std::wstring ConvertDriveInstanceIdToDevicePath(const std::wstring& deviceInstanceId) {
	std::wstring result = deviceInstanceId;

	// все символы в нижний регистр
	std::transform(result.begin(), result.end(), result.begin(), ::towlower);

	// должна содержать '\', '&ven_', '&prod_'
	if (result.find(L'\\') == std::wstring::npos ||
		result.find(L'&ven_') == std::wstring::npos ||
		result.find(L'&prod_') == std::wstring::npos) {
			return L"";
	}

	// заменяем все '\' на '#'
	std::replace(result.begin(), result.end(), L'\\', L'#');

	// добавляем в начало "\\?\"
	result.insert(0, L"\\\\?\\");

	// добавляем в конец GUID дискового устройства "#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}"
	result.append(L"#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}");

	return result;
}


// получить путь к hddwincmd_service.exe
bool GetServicePath(std::wstring& path, s_resp& resp) {

	wchar_t current_exe_path[MAX_PATH];
	DWORD length = GetModuleFileName(nullptr, current_exe_path, MAX_PATH);

	if (length <= 0) {
		WcoutExt(L"Failed to extract program path (GetModuleFileName)\r\n", resp, false);
		return false;
	}

	// последний обратный слэш
	wchar_t* lastSlash = wcsrchr(current_exe_path, L'\\');
	if (lastSlash == nullptr) {
		WcoutExt(L"Failed to extract program path (backslash search)\r\n", resp, false);
		return false;
	}
	*lastSlash = L'\0';
	path = current_exe_path;

	path += L"\\hddwincmd_service.exe";

	if (_waccess(path.c_str(), 0) != 0) {
		WcoutExt(L"This command requires [hddwincmd_service.exe] in the application folder to work\r\n", resp, false);
		return false;
	}

	return true;
}


// запустить процесс
bool RunApplication(const std::wstring& path, const std::wstring& parameters) {
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// формируем коммандную строку
	std::wstring modifiablePath = L"\"" + path + L"\"";
	if (parameters != L"") modifiablePath += L" " + parameters;

	// запускаем процесс
	if (CreateProcess(NULL, &modifiablePath[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}else {
		return false;
	}
}