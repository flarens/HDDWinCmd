#include <windows.h>
#include <iostream>
#include <string>

#include "functions.h"
#include "disk_info.h"
#include "disk_commands.h"
#include "help.h"


int main(int argc, char* argv[]) {
	// кодировка консоли
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	std::locale::global(std::locale(""));

	s_resp resp;
	resp.successAll = true;
	resp.type = 2;

	std::string arg_k, arg_p;
	std::wstring disk = L"";
	bool notclose = false;
	DWORD timeout = 20000;

	// без аргументов - список команд
	if (argc == 1) {
		WcoutExt(helpText, resp, true);
		notclose = true;
	}

	// если первая команда меняет тип вывода в консоль
	if (argc >= 3) {
		arg_k = argv[1];
		arg_p = argv[2];
		if (arg_k == "--r" || arg_k == "--response") {
			if (arg_p == "bin") resp.type = 1;
			else if (arg_p == "code") resp.type = 0;
			else if (arg_p == "hide") {
				resp.type = -1;
				HWND hWnd = GetConsoleWindow();
				if (hWnd != NULL) ShowWindow(hWnd, SW_HIDE);	// скрываем окно консоли
			}
		}
	}

	// интерпретация списка команд
	for (int k = 1; k < argc; ++k) {
		arg_k = argv[k];

		// команда начинается с "--"
		if (arg_k.rfind("--", 0) == 0) {

			// узнаем доступное количество параметров
			int params = 0;
			for (int p = k + 1; p < argc; ++p) {
				arg_p = argv[p];
				if (arg_p.rfind("--", 0) == 0) break;
				params++;
			}

			// показать текущую команду c параметрами
			if (resp.type > 1) {
				std::wstring str = StringToWstring(arg_k);
				for (int p = k + 1; p < argc; ++p) {
					if (p > k + params) break;
					str += L" " + StringToWstring(argv[p]);
				}
				std::wcout << L"[CMD]: " + str + L"\r\n";
			}

			// обработка конкретных команд:

			// помощь
			if (arg_k == "--h" || arg_k == "--help") {
				WcoutExt(helpText, resp, true);
			}

			// параметр путь к диску
			else if (arg_k == "--d" || arg_k == "--disk") {
				if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
				else { 
					disk = StringToWstring(argv[k + 1]); 
					WcoutExt(L"Assigned\r\n", resp, true);
				}
			}

			// параметр таймаут
			else if (arg_k == "--t" || arg_k == "--timeout") {
				if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
				else {
					std::wstring timeout_s;
					timeout_s = StringToWstring(argv[k + 1]);
					if (!IsDigitsOnly(timeout_s)) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter must be a number\r\n", resp, false);
					else if (timeout_s.size() > 9) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: maximum parameter length - 9 digits\r\n", resp, false);
					else {
						timeout = std::stoi(timeout_s);
						WcoutExt(L"Assigned\r\n", resp, true);
					}
				}
			}

			// инфо о диске
			else if (arg_k == "--i" || arg_k == "--info") {
				if (disk == L"") {
					GetPhysicalDrivesList(true, resp);
				}
				else {
					// получим соответствия дисков и их guid
					std::vector<DiskInfo> logicalDisks;
					GetLogicalVolumes(logicalDisks);
					// инфо о диске
					if (GetPhysicalDrive(GetPhysicalDriveFromDriveStr(disk), logicalDisks, true, resp, true)) WcoutExt(L"", resp, true);
					else WcoutExt(L"Could not get disk information: [" + disk + L"]\r\n", resp, false);
				}
			}

			// инфо о диске (кратко)
			else if (arg_k == "--im" || arg_k == "--infomin") {
				if (disk == L"") {
					GetPhysicalDrivesList(false, resp);
				}
				else {
					// получим соответствия дисков и их guid
					std::vector<DiskInfo> logicalDisks;
					GetLogicalVolumes(logicalDisks);
					// инфо о диске
					if (GetPhysicalDrive(GetPhysicalDriveFromDriveStr(disk), logicalDisks, false, resp, true)) WcoutExt(L"", resp, true);
					else WcoutExt(L"Could not get disk information: [" + disk + L"]\r\n", resp, false);
				}
			}

			// остановить/раскрутить шпиндель
			else if (arg_k == "--s" || arg_k == "--spin") {
				if (disk == L"") {
					WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: before executing the command, you must select the disk (using --disk)\r\n", resp, false);
				}else {
					if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
					else {
						arg_p = argv[k + 1];
						if (arg_p == "0" || arg_p == "spindown") DiskSpinDown(GetPhysicalDriveFromDriveStr(disk), timeout, resp);
						else if (arg_p == "1" || arg_p == "spinup") DiskSpinUp(GetPhysicalDriveFromDriveStr(disk), resp);
						else WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: unknown parameter [" + StringToWstring(arg_p) + L"]\r\n", resp, false);
					}
				}
			}

			// отключить/подключить диск
			else if (arg_k == "--p" || arg_k == "--plug") {
				if (disk == L"") {
					WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: before executing the command, you must select the disk (using --disk)\r\n", resp, false);
				}else {
					if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
					else {
						// флаги
						bool permanent = false;
						bool force = false;
						if (params >= 2) {
							for (int p = k + 2; p < argc; ++p) {
								if (p > k + params) break;
								arg_p = argv[p];
								if (arg_p == "reboot") permanent = false;
								else if (arg_p == "permanent") permanent = true;
								else if (arg_p == "safe") force = false;
								else if (arg_p == "force") force = true;
								else WcoutExt_Mini(L"Warning in [" + StringToWstring(arg_k) + L"] command: unknown parameter [" + StringToWstring(arg_p) + L"]\r\n", resp, true);
							}
						}
						// первый параметр
						arg_p = argv[k + 1];
						if (arg_p == "0" || arg_p == "offline") DiskGoOffline(GetPhysicalDriveFromDriveStr(disk), permanent, force, timeout, resp);
						else if (arg_p == "1" || arg_p == "online") DiskGoOnline(GetPhysicalDriveFromDriveStr(disk), permanent, resp);
						else WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: unknown parameter [" + StringToWstring(arg_p) + L"]\r\n", resp, false);
					}
				}
			}

			// ожидание
			else if (arg_k == "--w" || arg_k == "--wait") {
				if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
				else {
					std::wstring wait_s;
					wait_s = StringToWstring(argv[k + 1]);
					if (!IsDigitsOnly(wait_s)) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter must be a number\r\n", resp, false);
					else if (wait_s.size() > 9) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: maximum parameter length - 9 digits\r\n", resp, false);
					else {
						DWORD wait_i = std::stoi(wait_s);
						WcoutExt(L"Waiting " + wait_s + L" milliseconds...\r\n", resp, true);
						Sleep(wait_i);
					}
				}
			}

			// оставить консоль по окончанию
			else if (arg_k == "--nc" || arg_k == "--notclose") {
				if (notclose) WcoutExt(L"Already applied before\r\n", resp, true);
				else WcoutExt(L"Applied\r\n", resp, true);
				notclose = true;
			}

			// тип вывода в консоль
			else if (arg_k == "--r" || arg_k == "--response") {
				if (params == 0) WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: parameter required\r\n", resp, false);
				else {
					arg_p = argv[k + 1];
					int resp_type_old = resp.type;
					if (arg_p == "text") resp.type = 2;
					else if (arg_p == "bin") resp.type = 1;
					else if (arg_p == "code") resp.type = 0;
					else if (arg_p == "hide") {
						resp.type = -1;
						HWND hWnd = GetConsoleWindow();
						if (hWnd != NULL) ShowWindow(hWnd, SW_HIDE);	// скрываем окно консоли
					}
					else WcoutExt(L"Error in [" + StringToWstring(arg_k) + L"] command: unknown parameter [" + StringToWstring(arg_p) + L"]\r\n", resp, false);

					if (resp_type_old == -1 && resp.type > -1) {
						HWND hWnd = GetConsoleWindow();
						if (hWnd != NULL) ShowWindow(hWnd, SW_SHOW);	// покажем окно консоли
					}
				}
			}

			// неизвестная команда
			else WcoutExt(L"Unknown command: " + StringToWstring(arg_k) + L"\r\n", resp, false);

			WcoutExt_Mini(L"\r\n", resp, true);
			k = k + params;
		}
	}

	if (notclose && resp.type > -1) {
		WcoutExt_Mini(L"\r\nPress Enter to exit...\r\n", resp, true);
		std::cin.get();
	}

	if (resp.type <= 0 && !resp.successAll) return 1;

	return 0;
}
