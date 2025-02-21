#pragma once

#include <windows.h>
#include <string>

// вспомогательная структура для выодв ы консоль
struct s_resp {
	int type;
	bool successAll;
};

// выводит весь текст если response_type = text
void WcoutExt_Mini(const std::wstring& txt, s_resp& resp, const bool success);
// выводит текст при response_type = 2, выводит success/failure при response_type = 1
void WcoutExt(const std::wstring& txt, s_resp& resp, const bool success);

// проверка - строка только из цифр
bool IsDigitsOnly(const std::wstring& str);

// проверка - строка вида /dev/sda
bool IsLinuxDriveFormat(const std::wstring& str);

// проверка - одна латинская буква
bool IsOneLetter(const std::wstring& str);

// проверка - строка вида volume GUID
bool IsVolumeGUID(const std::wstring& str);

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

// распознать введенный drive
std::wstring GetPhysicalDriveFromDriveStr(const std::wstring& drive);

// получение номера диска	
int GetDriveNumber(const HANDLE& hDrive);

// конвертировать кол-во байт в удобочитаемый вид
std::wstring BytesToFormatString(long long bytes);

// код последней ошибки с расшифровкой
std::wstring GetLastErrorString();

// открыть диск
bool OpenDevice(HANDLE& hDevice, const std::wstring& devicePath, s_resp& resp);
