#include <windows.h>
#include <thread>
#include <mutex>
#include <vector>

#include <cfgmgr32.h>
#pragma comment(lib, "cfgmgr32.lib")


// коды ответа
enum HDDServ {
	HDDSERV_SUCCESS = 0,
	HDDSERV_UNKNOWN_COMMAND,
	HDDSERV_ALREADY_DELETED,
	HDDSERV_NOT_LOCATE_DEVNODE,
	HDDSERV_NOT_UNINSTALL_DEVNODE,
	HDDSERV_NOT_REMOVE_SUBTREE,
	HDDSERV_NOT_REENUMERATE_DEVNODE,
	HDDSERV_LINK_NOT_FOUND
};

// структука связи с диском
struct DiskLink {
	DEVINST devInst;
	wchar_t deviceInstanceId[MAX_PATH + 2];
};

void ServiceClient(HANDLE hPipe);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

std::vector<DiskLink> diskLinks;	// хранилище связей на все удаленные диски
std::mutex mtx;						// блокировка потоков
bool noexit = true;					// флаг завершения приложения
HANDLE hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	// событие прерывания подключения новых клиентов


// поток обслуживания клиента
void ServiceClient(HANDLE hPipe) {
	wchar_t buffer[MAX_PATH + 2] = { 0 };
	DWORD bytesRead;

	// ждем сообщение от клиента
	if (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL)) {

		unsigned char response = HDDSERV_SUCCESS;

		wchar_t command = buffer[0];

		// удаление первого символа строки
		for (int i = 0; buffer[i] != '\0'; ++i) {
			buffer[i] = buffer[i + 1];
		}

		// первый символ u - Uninstall (деинсталлировать диск); d - Delete (удалить диск)
		if (command == 'u' || command == 'd') {

			// если уже есть в списке удаленных
			for (const auto& link : diskLinks) {
				if (std::wcscmp(link.deviceInstanceId, buffer) == 0) {
					response = HDDSERV_ALREADY_DELETED;
					break;
				}
			}

			if (response == HDDSERV_SUCCESS) {
				// создаём новый элемент
				DiskLink newDiskLink;
				wcscpy_s(newDiskLink.deviceInstanceId, sizeof(buffer), buffer);
				
				// получаем дескриптор devInst для устройства DeviceInstanceId
				if (CM_Locate_DevNodeW(&newDiskLink.devInst, newDiskLink.deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS) {
					response = HDDSERV_NOT_LOCATE_DEVNODE;
				}

				// частичное удаление устройства
				// (не будит диск, не будится смартами, оставляет devicePath доступным, не восстанавливается через обновление конфигурации оборудования)
				if (response == HDDSERV_SUCCESS) {
					if (CM_Uninstall_DevNode(newDiskLink.devInst, 0) != CR_SUCCESS) {
						response = HDDSERV_NOT_UNINSTALL_DEVNODE;
					}
				}

				// добавляем связь
				if (response == HDDSERV_SUCCESS && command == 'u') {
					std::lock_guard<std::mutex> lock(mtx);
					diskLinks.push_back(newDiskLink);
					noexit = true;
				}
			}

		// первый символ r - Return (вернуть удаленный диск)
		}else if (command == 'r') {

			// ищем в списке связей
			bool finded = false;
			for (auto it = diskLinks.begin(); it != diskLinks.end(); ++it) {
				if (std::wcscmp(it->deviceInstanceId, buffer) == 0) {

					finded = true;

					// удаляем полностью устройство и его дочерние элементы, чтобы было доступно восстановление (будит диск)
					if (CM_Query_And_Remove_SubTree(it->devInst, nullptr, nullptr, 0, CM_REMOVE_UI_OK) != CR_SUCCESS) {
						response = HDDSERV_NOT_REMOVE_SUBTREE;
						break;
					}

					// восстановление устройства
					if (CM_Reenumerate_DevNode(it->devInst, 0) != CR_SUCCESS) {
						response = HDDSERV_NOT_REENUMERATE_DEVNODE;
						break;
					}

					// удаляем связь
					std::lock_guard<std::mutex> lock(mtx);
					diskLinks.erase(it);

					break;
				}
			}

			if (!finded) {
				response = HDDSERV_LINK_NOT_FOUND;
			}
			

		}
		else response = HDDSERV_UNKNOWN_COMMAND;
		
		// отправляем ответ
		DWORD bytesWritten;
		WriteFile(hPipe, &response, sizeof(response), &bytesWritten, NULL);

		// если связей больше нет
		std::lock_guard<std::mutex> lock(mtx);
		if (diskLinks.size() == 0) {
			noexit = false;			// задаем флаг на выход
			SetEvent(hStopEvent);	// отменяем ожидание новых подключений
			CloseHandle(hPipe);
			return;
		}
	}

	CloseHandle(hPipe);
}


// точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	// если приложение запущено независимо без HDDWinCmd
	if (strcmp(lpCmdLine, "fromHDDWinCmd") != 0) {
		MessageBoxW(
			NULL,
			L"This application runs automatically from HDDWinCmd to keep the descriptor to the deleted disk open. Click Ok to close this message.",
			L"Message",
			MB_OK | MB_ICONINFORMATION
		);
		return 0;
	}

	// защита от запуска дубликата программы
	HANDLE hMutex = CreateMutex(NULL, FALSE, L"HDDWinCmdServiceMutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 1;
	}

	const wchar_t* pipeName = L"\\\\.\\pipe\\HDDWinCmd";

	// проверка на уже открытый канал
	HANDLE checkPipe = CreateFileW(
		pipeName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	// уже существует
	if (checkPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(checkPipe);
		return 1;
	}

	// неожиданная ошибка при проверке
	if (GetLastError() != ERROR_FILE_NOT_FOUND) {
		return 1;
	}

	// основной цикл сервера
	while (noexit) {

		// создаем канал
		HANDLE hPipe = CreateNamedPipeW(
			pipeName,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			1024, 1024, 0, NULL
		);

		if (hPipe == INVALID_HANDLE_VALUE) {
			return 1;
		}

		// структура для асинхронного вызова ConnectNamedPipe
		OVERLAPPED overlapped = {};
		overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (overlapped.hEvent == NULL) {
			CloseHandle(hPipe);
			return 1;
		}

		// открытие канала
		BOOL result = ConnectNamedPipe(hPipe, &overlapped);
		if (!result && GetLastError() != ERROR_IO_PENDING) {
			CloseHandle(hPipe);
			CloseHandle(overlapped.hEvent);
			continue;
		}

		// ожидание подключения клиента или сигнала завершения
		HANDLE waitHandles[] = { overlapped.hEvent, hStopEvent };
		DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

		if (waitResult == WAIT_OBJECT_0) {
			// подключение успешно установлено
			std::thread clientThread(ServiceClient, hPipe);
			clientThread.detach();
		}else {
			// ожидание прервано
			std::lock_guard<std::mutex> lock(mtx);	// сначала пусть завершится поток клиента
			CloseHandle(hPipe);
		}

		CloseHandle(overlapped.hEvent);
	}

	CloseHandle(hStopEvent);
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	return 0;
}