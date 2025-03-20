#pragma once

#include <windows.h>
#include <string>

#include "logger.cpp"

// вспомогательная структура для выодв ы консоль
struct s_resp {
	int type;
	bool successAll;
	Logger logger;
};

// выводит весь текст если response_type = text
void WcoutExt_Mini(const std::wstring& txt, s_resp& resp, const bool success);
// выводит текст при response_type = 2, выводит success/failure при response_type = 1
void WcoutExt(const std::wstring& txt, s_resp& resp, const bool success);

// проверка - строка только из цифр
bool IsDigitsOnly(const std::wstring& str);

// проверка - строка вида /dev/sda
bool IsLinuxDriveFormat(const std::wstring& str);

// проверка - путь к диску вида \\.\PhysicalDriveX
bool IsPhysicalDrivePath(const std::wstring& str);

// проверка - латинская буква
bool IsLatinLetter(const wchar_t ch);

// проверка - одна латинская буква
bool IsOneLetter(const std::wstring& str);

// проверка - строка вида volume GUID
bool IsVolumeGUID(const std::wstring& str);

// проверка - строка вида DevicePath
bool IsDevicePath(const std::wstring& str);

// проверка - это файловый путь на диске
bool IsPath(const std::wstring& str);

// проверка - это корневой путь на диске, заданный буквой
bool IsLetterRootPath(const std::wstring& str);

// является ли путь корнем диска
int IsRootPath(std::wstring path);

// строка в многобайтовую строку
std::wstring StringToWstring(const char* str);
std::wstring StringToWstring(const std::string& str);

// преобразовать число в буквенное представление изчисления дисков Linux
std::wstring NumberTolinuxDriveLetters(int num);

// преобразование последовательности букв Linux диска в число
std::wstring LinuxDriveLettersToNumber(const std::wstring& input);

// конвертировать GUID в строку
std::wstring GUIDtoString(const GUID& guid);

// получить номер физического диска по volume GUID одного из его логических дисков
int GetPhysicalDriveFromVolumeGUID(std::wstring volumePathName);

// перейли к источнику символической ссылки
std::wstring GetSymbolicLinkTarget(const std::wstring& symlinkPath);

// получить родительскую директорию
std::wstring GetParentFolder(std::wstring path);

// трассировка пути к корню диска с учетом симлинков и точек монтирования
std::wstring TraseToRoot(const std::wstring& path);

// распознать введенный drive
std::wstring GetPhysicalDriveFromDriveStr(std::wstring drive);

// получение номера диска	
int GetDriveNumber(const HANDLE& hDrive);

// конвертировать кол-во байт в удобочитаемый вид
std::wstring BytesToFormatString(long long bytes);

// код последней ошибки с расшифровкой
std::wstring GetLastErrorString();

// открыть диск
bool OpenDevice(HANDLE& hDevice, const std::wstring& devicePath, s_resp& resp);

// преобразование DevicePath в DeviceInstanceId
std::wstring ConvertDevicePathToDeviceInstanceId(const std::wstring& devicePath);

// преобразование DeviceInstanceId дискового устройства в DevicePath
std::wstring ConvertDriveInstanceIdToDevicePath(const std::wstring& deviceInstanceId);

// получить путь к hddwincmd_service.exe
bool GetServicePath(std::wstring& path, s_resp& resp);

// запустить процесс
bool RunApplication(const std::wstring& path, const std::wstring& parameters);