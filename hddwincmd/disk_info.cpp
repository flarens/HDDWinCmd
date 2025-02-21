#include <windows.h>
#include <string>
#include <vector>
#include <setupapi.h>

#include "disk_info.h"
#include "functions.h"


// извлечь тип MBR раздела из базы строк 
std::wstring GetMBRPartitionType(const BYTE id) {
	if (std::strlen(MBRPartitionType[id]) == 0)  return L"Undefined";
	return StringToWstring(MBRPartitionType[id]);
}


// заполнить массив привязки логических дисков
std::vector<std::wstring> GetDriveLetters() {
	wchar_t driveStrings[256];
	DWORD length = GetLogicalDriveStringsW(sizeof(driveStrings) / sizeof(wchar_t), driveStrings);
	std::vector<std::wstring> drives;

	for (wchar_t* drive = driveStrings; *drive; drive += wcslen(drive) + 1) {
		drives.push_back(drive);
	}

	return drives;
}


// получение полей STORAGE_DEVICE_DESCRIPTOR в строки
std::wstring GetDescriptorString(BYTE* buffer, const DWORD offset) {
	if (offset != 0) {
		char* charcast = reinterpret_cast<char*>(buffer + offset);
		return std::wstring(charcast, charcast + strlen(charcast));
	}
	return L"unknown";
}


// получение информации о диске
void GetPhysicalDriveInfo(const HANDLE& hDrive, DriveInfo& drive) {
	BYTE buffer[1024] = { 0 };
	DWORD bytesReturned = 0;

	STORAGE_PROPERTY_QUERY query = {};
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;

	if (DeviceIoControl(
		hDrive,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&query,
		sizeof(query),
		&buffer,
		sizeof(buffer),
		&bytesReturned,
		NULL)) {
		STORAGE_DEVICE_DESCRIPTOR* descriptor = (STORAGE_DEVICE_DESCRIPTOR*)buffer;

		drive.busType = GetBusTypeString(descriptor->BusType);								// тип шины
		drive.product = GetDescriptorString(buffer, descriptor->ProductIdOffset);			// имя устройства
		drive.vendor = GetDescriptorString(buffer, descriptor->VendorIdOffset);				// поставщик
		drive.revision = GetDescriptorString(buffer, descriptor->ProductRevisionOffset);	// ревизия
		drive.serial = GetDescriptorString(buffer, descriptor->SerialNumberOffset);			// серийник
		// извлекаемый
		if (descriptor->RemovableMedia) drive.removable = L"true";
		else drive.removable = L"false";
		// очередь команд
		if (descriptor->CommandQueueing) drive.ncq = L"true";
		else drive.ncq = L"false";
	}else {
		drive.product = L"Error DeviceIoControl. " + GetLastErrorString();
		drive.vendor = drive.product;
		drive.revision = drive.product;
		drive.serial = drive.product;
		drive.removable = drive.product;
	}
}


// составить список всех логических дисков
bool GetLogicalVolumes(std::vector<DiskInfo>& logicalDisks) {
	wchar_t diskGUID[MAX_PATH];
	HANDLE hFind = FindFirstVolumeW(diskGUID, ARRAYSIZE(diskGUID));

	if (hFind == INVALID_HANDLE_VALUE) {
		// ошибка вызова FindFirstVolume - GetLastError()
		return false;
	}

	do {
		// уберем слеш с конца если есть
		size_t len = wcslen(diskGUID);
		if (len > 0 && diskGUID[len - 1] == L'\\') diskGUID[len - 1] = L'\0';

		// открываем том
		HANDLE hVolume = CreateFileW(
			diskGUID,	// ВАЖНО - без завершающего слеша!!!
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr
		);

		if (hVolume != INVALID_HANDLE_VALUE) {
			DWORD bytesReturned;
			VOLUME_DISK_EXTENTS diskExtents;
			if (DeviceIoControl(
				hVolume,
				IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
				nullptr,
				0,
				&diskExtents,
				sizeof(diskExtents),
				&bytesReturned,
				nullptr)) {
				for (DWORD i = 0; i < diskExtents.NumberOfDiskExtents; ++i) {
					DISK_EXTENT& extent = diskExtents.Extents[i];
					// возвращаем слеш в GUID
					if (len > 0) diskGUID[len - 1] = L'\\';
					// сохраняем идентификацию тома
					logicalDisks.push_back({ extent.DiskNumber, extent.StartingOffset.QuadPart, diskGUID });
				}
			}//else {ошибка получения информации о томе}
		}
		CloseHandle(hVolume);

	} while (FindNextVolumeW(hFind, diskGUID, ARRAYSIZE(diskGUID)));

	FindVolumeClose(hFind);
	return true;
}


// получить букву диска по volume GUID
std::wstring GetDriveLetterFromVolumeGUID(const std::wstring& volumeGUID) {
	WCHAR volumePath[MAX_PATH + 1] = { 0 };

	if (GetVolumePathNamesForVolumeNameW(volumeGUID.c_str(), volumePath, MAX_PATH, nullptr)) {

		if (volumePath[0] == L'\0') return L"not received";

		WCHAR volumeLabel[MAX_PATH + 1] = { 0 };
		DWORD volumeSerialNumber = 0;
		DWORD maxComponentLength = 0;
		DWORD fileSystemFlags = 0;
		WCHAR fileSystemName[MAX_PATH + 1] = { 0 };

		// получаем метку тома и файловую систему
		if (GetVolumeInformationW(
			volumePath,
			volumeLabel,
			ARRAYSIZE(volumeLabel),
			&volumeSerialNumber,
			&maxComponentLength,
			&fileSystemFlags,
			fileSystemName,
			ARRAYSIZE(fileSystemName))) {
			std::wstring info = std::wstring(volumePath);
			if (volumeLabel[0] != L'\0') info = info + L" (" + volumeLabel + L")";
			if (fileSystemName[0] != L'\0') info = info + L" [" + fileSystemName + L"]";
			return info;
		}else {
			return std::wstring(volumePath) + L" (volume label not received)";
		}
	}else {
		return L"not received";
	}
}


// получение геометрии диска
void GetDriveGeometry(const HANDLE& hDrive, std::wstring& geometry, std::wstring& size) {
	DISK_GEOMETRY diskGeometry;
	DWORD bytesReturned;

	// получаем информацию о диске
	if (!DeviceIoControl(
		hDrive,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&diskGeometry,
		sizeof(diskGeometry),
		&bytesReturned,
		NULL)) {
			geometry = L"Error getting disk geometry. " + GetLastErrorString();
			size = geometry;
			return;
	}

	geometry = L"Cylinders=" + std::to_wstring(diskGeometry.Cylinders.QuadPart) +
		L"; TracksPerCylinder=" + std::to_wstring(diskGeometry.TracksPerCylinder) +
		L"; SectorsPerTrack=" + std::to_wstring(diskGeometry.SectorsPerTrack) +
		L"; BytesPerSector=" + std::to_wstring(diskGeometry.BytesPerSector);

	// вычисляем полный размер диска
	LONGLONG diskSize;
	diskSize = diskGeometry.Cylinders.QuadPart *
		diskGeometry.TracksPerCylinder *
		diskGeometry.SectorsPerTrack *
		diskGeometry.BytesPerSector;

	size = std::to_wstring(diskSize) + L" bytes (~ " + BytesToFormatString(diskSize) + L")";
}


// получить путь диска по его номеру в системе
std::wstring GetPermanentDevicePathByDeviceNumber(int deviceNumber) {

	// получаем список всех устройств класса дисков
	HDEVINFO hDevInfo = SetupDiGetClassDevs(
		&GUID_DEVINTERFACE_DISK,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
	);

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return L"";
	}

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DWORD index = 0;
	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, index, &deviceInterfaceData)) {
		index++;

		// получаем размер буфера для деталей интерфейса
		DWORD requiredSize = 0;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

		std::vector<BYTE> buffer(requiredSize);
		auto* deviceInterfaceDetailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.data());
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		SP_DEVINFO_DATA devInfoData = {};
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		// получаем детали интерфейса
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, NULL, &devInfoData)) {
			continue;
		}

		// открываем диск
		HANDLE hDrive = CreateFileW(
			deviceInterfaceDetailData->DevicePath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		// если диск не найден
		if (hDrive == INVALID_HANDLE_VALUE) {
			continue;
		}

		// получение номера диска и путей
		int number = GetDriveNumber(hDrive);

		if (number == deviceNumber) {
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return std::wstring(deviceInterfaceDetailData->DevicePath);
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return L"";
}


// получить список дисков
void GetPhysicalDrivesList(bool maxinfo, s_resp& resp) {

	// получаем список всех устройств класса дисков
	HDEVINFO hDevInfo = SetupDiGetClassDevs(
		&GUID_DEVINTERFACE_DISK,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
	);

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		WcoutExt_Mini(L"Error receiving device list. " + GetLastErrorString() + L"\r\n", resp, false);
		return;
	}

	// получим соответствия дисков и их guid
	std::vector<DiskInfo> logicalDisks;
	GetLogicalVolumes(logicalDisks);

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	bool result = true;

	DWORD index = 0;
	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_DISK, index, &deviceInterfaceData)) {
		index++;

		// получаем размер буфера для деталей интерфейса
		DWORD requiredSize = 0;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

		std::vector<BYTE> buffer(requiredSize);
		auto* deviceInterfaceDetailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.data());
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		SP_DEVINFO_DATA devInfoData = {};
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		// получаем детали интерфейса
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, NULL, &devInfoData)) {
			WcoutExt_Mini(L"Error getting interface details for disk " + std::to_wstring(index) + L". " + GetLastErrorString() + L"\r\n", resp, false);
			continue;
		}

		if (!GetPhysicalDrive(deviceInterfaceDetailData->DevicePath, logicalDisks, maxinfo, resp, false))
			result = false;

	}

	SetupDiDestroyDeviceInfoList(hDevInfo);

	WcoutExt(L"", resp, result);
}


// получить информацию о диске
bool GetPhysicalDrive(const std::wstring& devicePath, std::vector<DiskInfo>& logicalDisks, const bool maxinfo, s_resp& resp, const bool single_source) {

	// открываем диск
	HANDLE hDevice;
	if (!OpenDevice(hDevice, devicePath, resp)) {
		return false;
	}

	// получение номера диска и путей
	int deviceNumber = GetDriveNumber(hDevice);
	if (deviceNumber == -1) {
		WcoutExt_Mini(L"\r\n======== DRIVE ? ========\r\nFailed to get the device number. " + GetLastErrorString() + L"\r\n", resp, true);
	}else {
		WcoutExt_Mini(L"\r\n======== DRIVE " + std::to_wstring(deviceNumber) + L" ========\r\n", resp, true);
		WcoutExt_Mini(L"temporary Windows path: \\\\.\\PhysicalDrive" + std::to_wstring(deviceNumber) + L"\r\n", resp, true);
		WcoutExt_Mini(L"temporary Linux path: /dev/sd" + NumberTolinuxDriveLetters(deviceNumber) + L"\r\n", resp, true);
		if (single_source) {
			std::wstring path;
			path = GetPermanentDevicePathByDeviceNumber(deviceNumber);
			if (path == L"") WcoutExt_Mini(L"permanent device path: unknown\r\n\r\n", resp, true);
			else WcoutExt_Mini(L"permanent device path:\r\n" + path + L"\r\n\r\n", resp, true);
		}
	}
	if (!single_source) WcoutExt_Mini(L"permanent device path:\r\n" + devicePath + L"\r\n\r\n", resp, true);

	// инфо о диске
	DriveInfo drive;
	GetPhysicalDriveInfo(hDevice, drive);
	WcoutExt_Mini(L"  bus type: " + drive.busType + L"\r\n", resp, true);
	WcoutExt_Mini(L"  product: " + drive.product + L"\r\n", resp, true);
	if (maxinfo) {
		WcoutExt_Mini(L"  vendor: " + drive.vendor + L"\r\n", resp, true);
		WcoutExt_Mini(L"  revision: " + drive.revision + L"\r\n", resp, true);
		WcoutExt_Mini(L"  serial number: " + drive.serial + L"\r\n", resp, true);
		WcoutExt_Mini(L"  removable: " + drive.removable + L"\r\n", resp, true);
		WcoutExt_Mini(L"  command queue support (NCQ): " + drive.ncq + L"\r\n", resp, true);
	}

	// геометрия диска
	std::wstring dGeometry;
	std::wstring dSize;
	GetDriveGeometry(hDevice, dGeometry, dSize);
	if (maxinfo) WcoutExt_Mini(L"  geometry: " + dGeometry + L"\r\n", resp, true);
	WcoutExt_Mini(L"  size: " + dSize + L"\r\n", resp, true);

	BYTE buffer[1024] = { 0 };
	DWORD bytesReturned = 0;

	// получение схемы логических дисков
	if (!DeviceIoControl(
		hDevice,
		IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
		NULL,
		0,
		buffer,
		sizeof(buffer),
		&bytesReturned,
		NULL)) {
		WcoutExt_Mini(L"Failed to get drive layout. " + GetLastErrorString() + L"\r\n", resp, false);
		CloseHandle(hDevice);
		return false;
	}
	DRIVE_LAYOUT_INFORMATION_EX* driveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)buffer;

	// получить Partition Table
	if (maxinfo) WcoutExt_Mini(L"  partition table: " + GetPartitionStyle(driveLayout->PartitionStyle) + L"\r\n", resp, true);

	WcoutExt_Mini(L"  partitions: " + std::to_wstring(driveLayout->PartitionCount) + L"\r\n", resp, true);

	if (driveLayout->PartitionStyle == PARTITION_STYLE_MBR || driveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
		for (DWORD i = 0; i < driveLayout->PartitionCount; ++i) {
			PARTITION_INFORMATION_EX& partition = driveLayout->PartitionEntry[i];
			std::wstring partitionNumber;
			if (partition.PartitionNumber > 0) partitionNumber = std::to_wstring(partition.PartitionNumber);
			else partitionNumber = L"?";
			WcoutExt_Mini(L"  " + partitionNumber + L".\r\n", resp, true);

			if (maxinfo && driveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
				WcoutExt_Mini(L"    GPT partition GUID: " + GUIDtoString(partition.Gpt.PartitionId) + L"\r\n", resp, true);
			}

			// поиск связки с логическим томом
			for (const auto& disk : logicalDisks) {
				if (disk.PhysicalDriveNumber == deviceNumber && disk.StartingOffset == partition.StartingOffset.QuadPart) {
					if (maxinfo) WcoutExt_Mini(L"    volume GUID path: " + disk.GUID + L"\r\n", resp, true);

					std::wstring driveLetter = GetDriveLetterFromVolumeGUID(disk.GUID);
					if (!driveLetter.empty()) {
						WcoutExt_Mini(L"    letter, label, fs: " + driveLetter + L"\r\n", resp, true);
					}else {
						WcoutExt_Mini(L"    letter: unavailable\r\n", resp, true);
					}
				}
			}

			if (maxinfo) WcoutExt_Mini(L"    start LBA: " + std::to_wstring(partition.StartingOffset.QuadPart) + L"\r\n", resp, true);
			WcoutExt_Mini(L"    size: " + std::to_wstring(partition.PartitionLength.QuadPart) + L" bytes (~ " + BytesToFormatString(partition.PartitionLength.QuadPart) + L")\r\n", resp, true);

			if (maxinfo && driveLayout->PartitionStyle == PARTITION_STYLE_MBR) {
				// тип партиции
				WcoutExt_Mini(L"    MBR partition type: " + GetMBRPartitionType(partition.Mbr.PartitionType) + L"\r\n", resp, true);
				// загрузочный или нет
				WcoutExt_Mini(L"    MBR boot partition: ", resp, true);
				if (partition.Mbr.BootIndicator) WcoutExt_Mini(L"true\r\n", resp, true);
				else WcoutExt_Mini(L"false\r\n", resp, true);
			}

			if (maxinfo && driveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
				// тип партиции
				WcoutExt_Mini(L"    GPT partition type: " + GetGPTPartitionType(partition.Gpt.PartitionType) + L"\r\n", resp, true);
				// имя партиции
				std::wstring wideName(partition.Gpt.Name);
				std::wstring partitionName(wideName.begin(), wideName.end());
				WcoutExt_Mini(L"    GPT partition name: " + partitionName + L"\r\n", resp, true);
				// список GPT атрибутов
				std::vector<std::wstring> attrstr;
				GetGPTAttributes(partition.Gpt.Attributes, attrstr);
				WcoutExt_Mini(L"    GPT attributes:\r\n", resp, true);
				for (const auto& attr : attrstr) {
					WcoutExt_Mini(L"    " + attr + L"\r\n", resp, true);
				}
			}
		}
	}

	WcoutExt_Mini(L"\r\n", resp, true);

	CloseHandle(hDevice);
	return true;
}


// получить volume GUID по букве логического диска
std::wstring GetVolumeGUIDFromDriveLetter(std::wstring driveLetter) {
	driveLetter = driveLetter + L":\\";

	wchar_t volumePathName[MAX_PATH];
	if (!GetVolumeNameForVolumeMountPoint(driveLetter.c_str(), volumePathName, MAX_PATH)) {
		return L"";
	}

	return std::wstring(volumePathName);
}


// получить Partition Table
std::wstring GetPartitionStyle(const DWORD pStyle) {
	switch (pStyle) {
	case PARTITION_STYLE_MBR:
		return L"MBR";
	case PARTITION_STYLE_GPT:
		return L"GPT";
	case PARTITION_STYLE_RAW:
		return L"RAW (No partition table)";
	}
	return L"Unknown";
}


// список GPT атрибутов
void GetGPTAttributes(const ULONGLONG attributes, std::vector<std::wstring>& str) {
	if (attributes & GPT_ATTRIBUTE_PLATFORM_REQUIRED) str.push_back(L"+ Platform required [active]");
	else str.push_back(L"- Platform required [NOT]");

	if (attributes & GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER) str.push_back(L"+ No Drive Letter [active]");
	else str.push_back(L"- No Drive Letter [NOT]");

	if (attributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) str.push_back(L"+ Hidden [active]");
	else str.push_back(L"- Hidden [NOT]");

	if (attributes & GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY) str.push_back(L"+ Shadow copy [active]");
	else str.push_back(L"- Shadow copy [NOT]");

	if (attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) str.push_back(L"+ Read-Only [active]");
	else str.push_back(L"- Read-Only [NOT]");
}


// тип шины
std::wstring GetBusTypeString(const STORAGE_BUS_TYPE busType) {
	switch (busType) {
	case 0x00: return L"Unknown";
	case 0x01: return L"SCSI";
	case 0x02: return L"ATAPI";
	case 0x03: return L"ATA";
	case 0x04: return L"IEEE 1394";
	case 0x05: return L"SSA";
	case 0x06: return L"Fibre Channel";
	case 0x07: return L"USB";
	case 0x08: return L"RAID";
	case 0x09: return L"iSCSI";
	case 0x0A: return L"SAS";
	case 0x0B: return L"SATA";
	case 0x0C: return L"SD";
	case 0x0D: return L"MMC";
	case 0x0E: return L"Virtual";
	case 0x0F: return L"File-Backed Virtual";
	case 0x10: return L"Storage Spaces";
	case 0x11: return L"NVMe";
	case 0x12: return L"SCM";
	case 0x13: return L"UFS";
	case 0x14: return L"NVMe-oF";
	case 0x15: return L"Max";
	case 0x7F: return L"Max Reserved";
	default: return L"Invalid";
	}
}


// определение GPT типа раздела https://ru.wikipedia.org/wiki/Таблица_разделов_GUID
std::wstring GetGPTPartitionType(const GUID id) {
	// без платформы
	if (IsEqualGUID(id, { 0x00000000, 0x0000, 0x0000,{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } })) return L"[without platform] Unused data record";
	else if (IsEqualGUID(id, { 0x024DEE41, 0x33E7, 0x11D3,{ 0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F } })) return L"[without platform] MBR partition scheme";
	else if (IsEqualGUID(id, { 0xC12A7328, 0xF81F, 0x11D2,{ 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B } })) return L"[without platform] EFI system partition";
	else if (IsEqualGUID(id, { 0x21686148, 0x6449, 0x6E6F,{ 0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } })) return L"[without platform] BIOS boot partition";
	else if (IsEqualGUID(id, { 0xD3BFE2DE, 0x3DAF, 0x11DF,{ 0xBA, 0x40, 0xE3, 0xA5, 0x56, 0xD8, 0x95, 0x93 } })) return L"[without platform] Intel Fast Flash (iFFS) Partition (for Intel Rapid Start Technology)";
	else if (IsEqualGUID(id, { 0xF4019732, 0x066E, 0x4E12,{ 0x82, 0x73, 0x34, 0x6C, 0x56, 0x41, 0x49, 0x4F } })) return L"[without platform] Sony boot partition";
	else if (IsEqualGUID(id, { 0xBFBFAFE7, 0xA34F, 0x448A,{ 0x9A, 0x5B, 0x62, 0x13, 0xEB, 0x73, 0x6C, 0x22 } })) return L"[without platform] Lenovo boot partition";
	// windows
	else if (IsEqualGUID(id, { 0xE3C9E316, 0x0B5C, 0x4DB8,{ 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE } })) return L"[Windows] Microsoft reserved partition (MSR)";
	else if (IsEqualGUID(id, { 0xEBD0A0A2, 0xB9E5, 0x4433,{ 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } })) return L"[Windows] Basic data partition";
	else if (IsEqualGUID(id, { 0x5808C8AA, 0x7E8F, 0x42E0,{ 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3 } })) return L"[Windows] Logical volume manager, metadata partition";
	else if (IsEqualGUID(id, { 0xAF9B60A0, 0x1431, 0x4F62,{ 0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD } })) return L"[Windows] Logical volume manager, data partition";
	else if (IsEqualGUID(id, { 0xDE94BBA4, 0x06D1, 0x4D40,{ 0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC } })) return L"[Windows] Microsoft recovery partition";
	else if (IsEqualGUID(id, { 0x45B0969E, 0x9B03, 0x4F30,{ 0xB4, 0x41, 0x32, 0x98, 0x28, 0x63, 0x0B, 0xE1 } })) return L"[Windows] Windows Recovery Environment (WinRE)";
	// Linux
	else if (IsEqualGUID(id, { 0x0FC63DAF, 0x8483, 0x4772,{ 0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 } })) return L"[Linux] Data partition";
	else if (IsEqualGUID(id, { 0xA19D880F, 0x05FC, 0x4D3B,{ 0xA0, 0x06, 0x74, 0x3F, 0x0F, 0x84, 0x91, 0x1E } })) return L"[Linux] RAID partition";
	else if (IsEqualGUID(id, { 0x0657FD6D, 0xA4AB, 0x43C4,{ 0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F } })) return L"[Linux] Swap partition";
	else if (IsEqualGUID(id, { 0xE6D6D379, 0xF507, 0x44C2,{ 0xA2, 0x3C, 0x23, 0x8F, 0x2A, 0x3D, 0xF9, 0x28 } })) return L"[Linux] Logical Volume Manager (LVM) partition";
	else if (IsEqualGUID(id, { 0x933AC7E1, 0x2EB4, 0x4F13,{ 0xB8, 0x44, 0x0E, 0x14, 0xE2, 0xAE, 0xF9, 0x15 } })) return L"[Linux] Partition /home";
	else if (IsEqualGUID(id, { 0x3B8F8425, 0x20E0, 0x4F3B,{ 0x90, 0x7F, 0x1A, 0x25, 0xA7, 0x6F, 0x98, 0xE8 } })) return L"[Linux] Partition /srv (server data)";
	else if (IsEqualGUID(id, { 0x7FFEC5C9, 0x2D00, 0x49B7,{ 0x89, 0x41, 0x3E, 0xA1, 0x0A, 0x55, 0x86, 0xB7 } })) return L"[Linux] Partition dm-crypt";
	else if (IsEqualGUID(id, { 0xCA7D7CCB, 0x63ED, 0x4C53,{ 0x86, 0x1C, 0x17, 0x42, 0x53, 0x60, 0x59, 0xCC } })) return L"[Linux] Partition LUKS";
	else if (IsEqualGUID(id, { 0x8DA63339, 0x0007, 0x60C0,{ 0xC4, 0x36, 0x08, 0x3A, 0xC8, 0x23, 0x09, 0x08 } })) return L"[Linux] Reserved";
	// FreeBSD
	else if (IsEqualGUID(id, { 0x83BD6B9D, 0x7F41, 0x11DC,{ 0xBE, 0x0B, 0x00, 0x15, 0x60, 0xB8, 0x4F, 0x0F } })) return L"[FreeBSD] Boot partition";
	else if (IsEqualGUID(id, { 0x516E7CB4, 0x6ECF, 0x11D6,{ 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B } })) return L"[FreeBSD] Data partition";
	else if (IsEqualGUID(id, { 0x516E7CB5, 0x6ECF, 0x11D6,{ 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B } })) return L"[FreeBSD] Swap partition";
	else if (IsEqualGUID(id, { 0x516E7CB6, 0x6ECF, 0x11D6,{ 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B } })) return L"[FreeBSD] UFS (Unix File System) partition";
	else if (IsEqualGUID(id, { 0x516E7CB8, 0x6ECF, 0x11D6,{ 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B } })) return L"[FreeBSD] Vinum volume manager partition";
	else if (IsEqualGUID(id, { 0x516E7CBA, 0x6ECF, 0x11D6,{ 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B } })) return L"[FreeBSD] ZFS partition";
	// macOS
	else if (IsEqualGUID(id, { 0x48465300, 0x0000, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] HFS+ (Hierarchical File System) partition";
	else if (IsEqualGUID(id, { 0x7C3457EF, 0x0000, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] APFS (Apple File System) partition";
	else if (IsEqualGUID(id, { 0x55465300, 0x0000, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple UFS";
	else if (IsEqualGUID(id, { 0x6A898CC3, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[macOS] ZFS";
	else if (IsEqualGUID(id, { 0x52414944, 0x0000, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple RAID partition";
	else if (IsEqualGUID(id, { 0x52414944, 0x5F4F, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple RAID partition, offline";
	else if (IsEqualGUID(id, { 0x426F6F74, 0x0000, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple boot partition";
	else if (IsEqualGUID(id, { 0x4C616265, 0x6C00, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple Label";
	else if (IsEqualGUID(id, { 0x5265636F, 0x7665, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple TV Recovery partition";
	else if (IsEqualGUID(id, { 0x53746F72, 0x6167, 0x11AA,{ 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } })) return L"[macOS] Apple Core Storage partition (Lion FileVault)";
	// Solaris
	else if (IsEqualGUID(id, { 0x6A82CB45, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Boot partition";
	else if (IsEqualGUID(id, { 0x6A85CF4D, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Root partition";
	else if (IsEqualGUID(id, { 0x6A87C46F, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Swap partition";
	else if (IsEqualGUID(id, { 0x6A8B642B, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Backup partition";
	else if (IsEqualGUID(id, { 0x6A898CC3, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Partition /usr";
	else if (IsEqualGUID(id, { 0x6A8EF2E9, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Partition /var";
	else if (IsEqualGUID(id, { 0x6A90BA39, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Partition /home";
	else if (IsEqualGUID(id, { 0x6A9283A5, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] EFI_ALTSCTR";
	else if (IsEqualGUID(id, { 0x6A945A3B, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Reserved";
	else if (IsEqualGUID(id, { 0x6A9630D1, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Reserved";
	else if (IsEqualGUID(id, { 0x6A980767, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Reserved";
	else if (IsEqualGUID(id, { 0x6A96237F, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Reserved";
	else if (IsEqualGUID(id, { 0x6A8D2AC7, 0x1DD2, 0x11B2,{ 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } })) return L"[Solaris] Reserved";
	// NetBSD
	else if (IsEqualGUID(id, { 0x49F48D32, 0xB10E, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] Swap partition";
	else if (IsEqualGUID(id, { 0x49F48D5A, 0xB10E, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] FFS partition (Fast File System)";
	else if (IsEqualGUID(id, { 0x49F48D82, 0xB10E, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] LFS partition (Log-structured File System)";
	else if (IsEqualGUID(id, { 0x49F48DAA, 0xB10E, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] RAID partition";
	else if (IsEqualGUID(id, { 0x2DB519C4, 0xB10F, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] Connected partition";
	else if (IsEqualGUID(id, { 0x2DB519EC, 0xB10F, 0x11DC,{ 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48 } })) return L"[NetBSD] Encrypted partition";
	// Chrome OS
	else if (IsEqualGUID(id, { 0xFE3A2A5D, 0x4F32, 0x41A7,{ 0xB7, 0x25, 0xAC, 0xCC, 0x32, 0x85, 0xA3, 0x09 } })) return L"[Chrome OS] Kernel";
	else if (IsEqualGUID(id, { 0x3CB8E202, 0x3B7E, 0x47DD,{ 0x8A, 0x3C, 0x7F, 0xF2, 0xA1, 0x3C, 0xFC, 0xEC } })) return L"[Chrome OS] Rootfs";
	else if (IsEqualGUID(id, { 0x2E0A753D, 0x9E48, 0x43B0,{ 0x83, 0x37, 0xB1, 0x51, 0x92, 0xCB, 0x1B, 0x5E } })) return L"[Chrome OS] For future reference";
	// другие
	else if (IsEqualGUID(id, { 0x75894C1E, 0x3AEB, 0x11D3,{ 0xB7, 0xC1, 0x7B, 0x03, 0xA0, 0x00, 0x00, 0x00 } })) return L"[HP-UX] Data partition";
	else if (IsEqualGUID(id, { 0xE2A1E728, 0x32E3, 0x11D6,{ 0xA6, 0x82, 0x7B, 0x03, 0xA0, 0x00, 0x00, 0x00 } })) return L"[HP-UX] Service partition";
	else if (IsEqualGUID(id, { 0xCEF5A9AD, 0x73BC, 0x4601,{ 0x89, 0xF3, 0xCD, 0xEE, 0xEE, 0xE3, 0x21, 0xA1 } })) return L"[QNX] Power-safe (QNX6) file system";
	else if (IsEqualGUID(id, { 0x90B6FF38, 0xB98F, 0x4358,{ 0xA2, 0x1F, 0x48, 0xF3, 0x5B, 0x4A, 0x8A, 0xD3 } })) return L"[OS/2] ArcaOS Type 1";

	return L"Undefined";
}
