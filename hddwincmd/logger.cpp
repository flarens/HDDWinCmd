#pragma once

#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>

// статусы
enum LogStatus {
	LOG_STATUS_EMPTY = 0,
	LOG_STATUS_SUCCESS,
	LOG_STATUS_ERROR,
	LOG_STATUS_OFF,
	LOG_STATUS_INCORRECT,
	LOG_STATUS_EQUAL,
	LOG_STATUS_BANNED_FILE
};


class Logger {
public:

	Logger() {
		status = LOG_STATUS_EMPTY;
	}

	~Logger() {
		CloseFile();
	}

	// установка лог файла
	void SetLogFile(const std::wstring& filename) {

		status = LOG_STATUS_EMPTY;

		// пустое имя файла
		if (filename == L"") {
			CloseFile();
			openedFilename = filename;
			status = LOG_STATUS_OFF;
			return;
		}
		
		// имя уже открытого файла
		if (filename == openedFilename) {
			status = LOG_STATUS_EQUAL;
			return;
		}

		// проверка на запрещенные символы
		if (!IsValidFilePath(filename)) {
			status = LOG_STATUS_INCORRECT;
			return;
		}

		// запрещенные расширения
		if (filename.size() > 4 && _wcsicmp(filename.c_str() + filename.size() - 4, L".exe") == 0) {
			status = LOG_STATUS_BANNED_FILE;
			return;
		}

		// подключаем
		CloseFile();
		file.open(filename.c_str(), std::ios::app | std::ios::binary);
		if (!file.is_open()) {
			status = LOG_STATUS_ERROR;
			return;
		}

		// получаем текущее время
		auto now = std::chrono::system_clock::now();
		std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime;
		localtime_s(&localTime, &currentTime);

		// штамп времени
		file << L"====================================\r\n= SESSION FROM " 
			<< std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S") 
			<< L" =\r\n====================================\r\n\r\n";
		file.flush();

		openedFilename = filename;
		status = LOG_STATUS_SUCCESS;
	}

	// запись строки в файл
	void Log(const std::wstring& message) {
		if (file.is_open()) {
			file << message;
			file.flush();
		}
	}

	// получить результат последней установки лог файла
	std::wstring GetStatus() {
		switch (status) {
			case LOG_STATUS_EMPTY: return L"The log file is not initialized";
			case LOG_STATUS_SUCCESS: return L"Successfully assigned a log file";
			case LOG_STATUS_ERROR: return L"Failed to connect log file";
			case LOG_STATUS_OFF: return L"Logging is disabled";
			case LOG_STATUS_INCORRECT: return L"The file name contains illegal characters. Only Latin letters, numbers and the character set _.-@#$%&\'()+,;=[]\\ are allowed";
			case LOG_STATUS_EQUAL: return L"The selected log file matches an already active log file";
			case LOG_STATUS_BANNED_FILE: return L"This file extension is not allowed for use";
			default: return L"Unknown error";
		}
	}

private:
	std::wofstream file;
	std::wstring openedFilename;
	int status;

	// закрыть файл
	void CloseFile() {
		if (file.is_open()) {
			file.close();
		}
	}

	// проверка корректности пути файла
	bool IsValidFilePath(const std::wstring& path) {
		for (wchar_t ch : path) {

			// проверка, что буквы принадлежат только латинскому алфавиту
			if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) {
				continue; // Латинские буквы разрешены
			}

			// разрешаем цифры и другие допустимые символы
			if ((ch >= L'0' && ch <= L'9') ||
				ch == L'_' || ch == L'.' || ch == L'-' ||
				ch == L'@' || ch == L'#' || ch == L'$' ||
				ch == L'%' || ch == L'&' || ch == L'\'' ||
				ch == L'(' || ch == L')' || ch == L'+' ||
				ch == L',' || ch == L';' || ch == L'=' ||
				ch == L'[' || ch == L']' || ch == L'\\') {
				continue;
			}

			return false;
		}

		return true;
	}

};