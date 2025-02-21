#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include "functions.h"

// структука информации об устройстве
struct DriveInfo {
	std::wstring busType;
	std::wstring product;
	std::wstring vendor;
	std::wstring revision;
	std::wstring serial;
	std::wstring removable;
	std::wstring ncq;
};

// структука привязка логического диска
struct DiskInfo {
	unsigned long PhysicalDriveNumber;
	long long StartingOffset;
	std::wstring GUID;
};

// извлечь тип MBR раздела из базы строк
std::wstring GetMBRPartitionType(const BYTE id);

// заполнить массив привязки логических дисков
std::vector<std::wstring> GetDriveLetters();

// получение полей STORAGE_DEVICE_DESCRIPTOR в строки
std::wstring GetDescriptorString(BYTE* buffer, const DWORD offset);

// получение информации о диске
void GetPhysicalDriveInfo(const HANDLE& hDrive, DriveInfo& drive);

// составить список всех логических дисков
bool GetLogicalVolumes(std::vector<DiskInfo>& logicalDisks);

// получить букву диска по volume GUID
std::wstring GetDriveLetterFromVolumeGUID(const std::wstring& volumeGUID);

// получение геометрии диска
void GetDriveGeometry(const HANDLE& hDrive, std::wstring& geometry, std::wstring& size);

// получить путь диска по его номеру в системе	
std::wstring GetPermanentDevicePathByDeviceNumber(int deviceNumber);

// получить список дисков
void GetPhysicalDrivesList(bool maxinfo, s_resp& resp);

// получить информацию о диске
bool GetPhysicalDrive(const std::wstring& physicalDrive, std::vector<DiskInfo>& logicalDisks, const bool maxinfo, s_resp& resp, const bool single_source);

// получить volume GUID по букве логического диска
std::wstring GetVolumeGUIDFromDriveLetter(std::wstring driveLetter);

// получить Partition Table
std::wstring GetPartitionStyle(const DWORD pStyle);

// список GPT атрибутов
void GetGPTAttributes(const ULONGLONG attributes, std::vector<std::wstring>& str);

// тип шины
std::wstring GetBusTypeString(const STORAGE_BUS_TYPE busType);

// определение GPT типа раздела
std::wstring GetGPTPartitionType(const GUID id);


// база строк MBR типа раздела https://en.wikipedia.org/wiki/Partition_type
static constexpr const char* const MBRPartitionType[] = {
	/* 00 */ "[without platform] Empty partition entry",
	/* 01 */ "[DOS 2.0+] FAT12 file system partition",
	/* 02 */ "[XENIX] XENIX root",
	/* 03 */ "[XENIX] XENIX usr",
	/* 04 */ "[DOS 3.0+] FAT16 file system partition",
	/* 05 */ "[Some versions of DOS 3.2, DOS 3.3+] Extended partition with CHS addressing",
	/* 06 */ "[DOS 3.31+] FAT16B file system partition",
	/* 07 */ "[OS/2 1.2+ / Windows NT / Windows Embedded CE / QNX 2] IFS / HPFS / NTFS / exFAT / QNX",
	/* 08 */ "[Commodore MS-DOS 3.x / OS/2 1.0-1.3 / AIX / QNX 1.x/2.x] Logical sectored FAT12 or FAT16 / OS/2 (FAT?) / AIX boot/split / QNX / Partition spanning multiple drives",
	/* 09 */ "[AIX / QNX 1.x/2.x / Coherent / OS-9] AIX data/boot / QNX / Coherent file system / OS-9 RBF",
	/* 0A */ "[OS/2 / Coherent] OS/2 Boot Manager / Coherent swap partition",
	/* 0B */ "[DOS 7.1+] FAT32 with CHS addressing",
	/* 0C */ "[DOS 7.1+] FAT32 with LBA",
	/* 0D */ "",
	/* 0E */ "[DOS 7.0+] FAT16B with LBA",
	/* 0F */ "[DOS 7.0+] Extended partition with LBA",
	/* 10 */ "",
	/* 11 */ "[Leading Edge MS-DOS 3.x/ OS/2 Boot Manager] Logical sectored FAT12 or FAT16 / Hidden FAT12",
	/* 12 */ "[EISA machines / Compaq Contura] Configuration partition / Recovery partition / EISA configuration utility for the system / Hibernation partition / Diagnostics and firmware partition / Service partition / Rescue and Recovery partition",
	/* 13 */ "",
	/* 14 */ "[AST MS-DOS 3.x / OS/2 Boot Manager / Maverick OS] Logical sectored FAT12 or FAT16 / Hidden FAT16",
	/* 15 */ "[OS/2 Boot Manager / Maverick OS] Hidden extended partition with CHS addressing / Swap",
	/* 16 */ "[OS/2 Boot Manager] Hidden FAT16B",
	/* 17 */ "[OS/2 Boot Manager] Hidden IFS/HPFS/NTFS/exFAT",
	/* 18 */ "[AST Windows] AST Zero Volt Suspend or SmartSleep partition",
	/* 19 */ "[Willowtech Photon coS] Willowtech Photon coS",
	/* 1A */ "",
	/* 1B */ "[OS/2 Boot Manager] Hidden FAT32",
	/* 1C */ "[OS/2 Boot Manager / ASUS eRecovery] Hidden FAT32 with LBA / ASUS recovery partition",
	/* 1D */ "",
	/* 1E */ "[OS/2 Boot Manager] Hidden FAT16 with LBA",
	/* 1F */ "[OS/2 Boot Manager] Hidden extended partition with LBA addressing",
	/* 20 */ "[Windows Mobile] Windows Mobile update XIP / Willowsoft Overture File System (OFS1)",
	/* 21 */ "[HP Volume Expansion / Oxygen] FSo2 (Oxygen File System)",
	/* 22 */ "[Oxygen] Oxygen Extended Partition Table",
	/* 23 */ "[Windows Mobile] Windows Mobile boot XIP",
	/* 24 */ "[NEC MS-DOS 3.30] Logical sectored FAT12 or FAT16",
	/* 25 */ "",
	/* 26 */ "",
	/* 27 */ "[Windows / D2D eRecovery / Linux (RouterBOARD 500)] Windows Recovery Environment (RE) partition / Rescue partition / RooterBOOT kernel partition",
	/* 28 */ "",
	/* 29 */ "",
	/* 2A */ "[AtheOS] AtheOS file system (AthFS, AFS)",
	/* 2B */ "[SyllableOS] SyllableSecure (SylStor), a variant of AthFS",
	/* 2C */ "",
	/* 2D */ "",
	/* 2E */ "",
	/* 2F */ "",
	/* 30 */ "[Personal CP/M-86] Found in some OEM Siemens systems instead of DBh. Filesystem parameters are in the following sector (LBA 1), not the partition itself",
	/* 31 */ "Reserved",
	/* 32 */ "",
	/* 33 */ "Reserved",
	/* 34 */ "Reserved",
	/* 35 */ "[OS/2 Warp Server / eComStation] JFS (OS/2 implementation of AIX Journaling File system)",
	/* 36 */ "Reserved",
	/* 37 */ "",
	/* 38 */ "[THEOS] THEOS version 3.2, 2 GB partition",
	/* 39 */ "[Plan 9 / THEOS] Plan 9 edition 3 partition (sub-partitions described in second sector of partition) / THEOS version 4 spanned partition",
	/* 3A */ "[THEOS] THEOS version 4, 4 GB partition",
	/* 3B */ "[THEOS] THEOS version 4 extended partition",
	/* 3C */ "[PartitionMagic] PqRP (PartitionMagic or DriveImage in progress)",
	/* 3D */ "[PartitionMagic] Hidden NetWare",
	/* 3E */ "",
	/* 3F */ "",
	/* 40 */ "[PICK / Venix] PICK R83 / Venix 80286",
	/* 41 */ "[Personal RISC / Linux / PowerPC] Personal RISC Boot / Old Linux/Minix (disk shared with DR DOS 6.0) / PPC PReP (Power PC Reference Platform) Boot",
	/* 42 */ "[SFS / Linux / Windows 2000, XP, etc.] Secure File system (SFS) / Old Linux swap (disk shared with DR DOS 6.0) / Dynamic extended partition marker",
	/* 43 */ "[Linux] Old Linux native (disk shared with DR DOS 6.0)",
	/* 44 */ "[GoBack] Norton GoBack, WildFile GoBack, Adaptec GoBack, Roxio GoBack",
	/* 45 */ "[Boot-US / EUMEL/ELAN] Priam / Boot-US boot manager (1 cylinder) / EUMEL/ELAN (L2)",
	/* 46 */ "[EUMEL/ELAN] EUMEL/ELAN (L2)",
	/* 47 */ "[EUMEL/ELAN] EUMEL/ELAN (L2)",
	/* 48 */ "[EUMEL/ELAN] EUMEL/ELAN (L2), ERGOS L3",
	/* 49 */ "",
	/* 4A */ "[AdaOS / ALFS/THIN] Aquila / ALFS/THIN advanced lightweight file system for DOS",
	/* 4B */ "",
	/* 4C */ "[ETH Oberon] Aos (A2) file system (76)",
	/* 4D */ "[QNX 4.x, Neutrino] Primary QNX POSIX volume on disk (77)",
	/* 4E */ "[QNX 4.x, Neutrino] Secondary QNX POSIX volume on disk (78)",
	/* 4F */ "[QNX 4.x, Neutrino / ETH Oberon] Tertiary QNX POSIX volume on disk (79) / Boot/native file system (79)",
	/* 50 */ "[ETH Oberon / Disk Manager 4 / LynxOS / Novell] Alternative native file system (80) / Read-only partition (old) / Lynx RTOS",
	/* 51 */ "[Disk Manager 4-6] Read-write partition (Aux 1)",
	/* 52 */ "[CP/M-80 / System V/AT, V/386] CP/M-80",
	/* 53 */ "[Disk Manager 6] Auxiliary 3 (WO)",
	/* 54 */ "[Disk Manager 6] Dynamic Drive Overlay (DDO)",
	/* 55 */ "[EZ-Drive] EZ-Drive, Maxtor, MaxBlast, or DriveGuide INT 13h redirector volume",
	/* 56 */ "[AT&T MS-DOS 3.x / EZ-Drive / VFeature] Logical sectored FAT12 or FAT16 / Disk Manager partition converted to EZ-BIOS / VFeature partitioned volume",
	/* 57 */ "[DrivePro] VNDI partition",
	/* 58 */ "",
	/* 59 */ "[yocOS] yocFS",
	/* 5A */ "",
	/* 5B */ "",
	/* 5C */ "[EDISK] Priam EDisk Partitioned Volume",
	/* 5D */ "",
	/* 5E */ "",
	/* 5F */ "",
	/* 60 */ "",
	/* 61 */ "[SpeedStor] Hidden FAT12",
	/* 62 */ "",
	/* 63 */ "[SCO Unix, ISC, UnixWare, AT&T System V/386, ix, MtXinu BSD 4.3 on Mach / GNU/Hurd / SpeedStor] Old GNU/Hurd with UFS support / Hidden read-only FAT12",
	/* 64 */ "[SpeedStor / NetWare] Hidden FAT16 / NetWare File System 286/2 / PC-ARMOUR",
	/* 65 */ "[NetWare] NetWare File System 386",
	/* 66 */ "[NetWare / SpeedStor] Storage Management Services (SMS) / Hidden read-only FAT16",
	/* 67 */ "[NetWare] Wolf Mountain cluster",
	/* 68 */ "[NetWare] -",
	/* 69 */ "[NetWare / NetWare 5] Novell Storage Services (NSS)",
	/* 6A */ "",
	/* 6B */ "",
	/* 6C */ "[BSD] BSD slice (DragonFly BSD)",
	/* 6D */ "",
	/* 6E */ "",
	/* 6F */ "",
	/* 70 */ "[DiskSecure] DiskSecure multiboot",
	/* 71 */ "Reserved",
	/* 72 */ "[APTI conformant systems / Unix V7/x86] APTI alternative FAT12 (CHS, SFN) / V7/x86",
	/* 73 */ "Reserved",
	/* 74 */ "[SpeedStor] Hidden FAT16B",
	/* 75 */ "[PC/IX] -",
	/* 76 */ "[SpeedStor] Hidden read-only FAT16B",
	/* 77 */ "[Novell] VNDI, M2FS, M2CS",
	/* 78 */ "[Geurt Vos] XOSL bootloader file system",
	/* 79 */ "[APTI conformant systems] APTI alternative FAT16 (CHS, SFN)",
	/* 7A */ "[APTI conformant systems] APTI alternative FAT16 (LBA, SFN)",
	/* 7B */ "[APTI conformant systems] APTI alternative FAT16B (CHS, SFN)",
	/* 7C */ "[APTI conformant systems] APTI alternative FAT32 (LBA, SFN)",
	/* 7D */ "[APTI conformant systems] APTI alternative FAT32 (CHS, SFN)",
	/* 7E */ "[PrimoCache] Level 2 cache",
	/* 7F */ "[Alternative OS Development Partition Standard] Reserved for individual or local use and temporary or experimental projects",
	/* 80 */ "[Minix 1.1-1.4a] MINIX file system (old)",
	/* 81 */ "[Minix 1.4b+] MINIX file system",
	/* 82 */ "[Linux / GNU/Hurd / Sun Microsystems] Linux swap space / GNU/Hurd (Hurd uses the same Linux swap file system) / Solaris x86 (for Sun disklabels up to 2005)",
	/* 83 */ "[Linux / GNU/Hurd] Any native Linux file system / GNU/Hurd",
	/* 84 */ "[OS/2 / Rapid Start technology] APM hibernation / Hidden C: (FAT16) / Rapid Start hibernation data (possibly iFFS; possibly used for Intel SRT SSD cache as well)",
	/* 85 */ "[Linux] Linux extended",
	/* 86 */ "[Windows NT 4 Server / Linux] Fault-tolerant FAT16B mirrored volume set / Linux RAID superblock with auto-detect (old)",
	/* 87 */ "[Windows NT 4 Server] Fault-tolerant HPFS/NTFS mirrored volume set",
	/* 88 */ "[Linux] Linux plaintext partition table",
	/* 89 */ "",
	/* 8A */ "[AirBoot] AirBoot is a track0 Boot Manager with on-the-fly partition detection",
	/* 8B */ "[Windows NT 4 Server] Legacy fault-tolerant FAT32 mirrored volume set",
	/* 8C */ "[Windows NT 4 Server] Legacy fault-tolerant FAT32 mirrored volume set",
	/* 8D */ "[Free FDISK] Hidden FAT12",
	/* 8E */ "[Linux] Linux LVM since 1999",
	/* 8F */ "",
	/* 90 */ "[Free FDISK] Hidden FAT16",
	/* 91 */ "[Free FDISK] Hidden extended partition with CHS addressing",
	/* 92 */ "[Free FDISK] Hidden FAT16B",
	/* 93 */ "[Amoeba / Linux] Amoeba native file system / Hidden Linux file system",
	/* 94 */ "[Amoeba] Amoeba bad block table",
	/* 95 */ "[EXOPC] EXOPC native",
	/* 96 */ "[CHRP] ISO-9660 file system",
	/* 97 */ "[Free FDISK] Hidden FAT32",
	/* 98 */ "[Free FDISK / ROM-DOS] Hidden FAT32 / Service partition (bootable FAT) ROM-DOS SuperBoot / Service partition",
	/* 99 */ "[?] Early Unix",
	/* 9A */ "[Free FDISK] Hidden FAT16",
	/* 9B */ "[Free FDISK] Hidden extended partition with LBA",
	/* 9C */ "",
	/* 9D */ "",
	/* 9E */ "[VSTa / ForthOS] ForthOS (eForth port",
	/* 9F */ "[BSD/OS 3.0+, BSDI] ?",
	/* A0 */ "[?] Diagnostic partition for HP laptops / Hibernate partition",
	/* A1 */ "[HP Volume Expansion] Hibernate partition",
	/* A2 */ "[Cyclone V] Hard Processor System (HPS) ARM preloader",
	/* A3 */ "[HP Volume Expansion] ?",
	/* A4 */ "[HP Volume Expansion] ?",
	/* A5 */ "[BSD] BSD slice (BSD/386, 386BSD, NetBSD (before 1998-02-19), FreeBSD)",
	/* A6 */ "[HP Volume Expansion / OpenBSD] ? / OpenBSD slice",
	/* A7 */ "[NeXTSTEP] ?",
	/* A8 */ "[Darwin, Mac OS X] Apple Darwin, Mac OS X UFS",
	/* A9 */ "[NetBSD] NetBSD slice",
	/* AA */ "[MS-DOS] Olivetti MS-DOS FAT12 (1.44 MB)",
	/* AB */ "[Darwin, Mac OS X / GO! OS] Apple Darwin, Mac OS X boot / GO!",
	/* AC */ "[Darwin, Mac OS X] Apple RAID, Mac OS X RAID",
	/* AD */ "[RISC OS] ADFS / FileCore format",
	/* AE */ "[ShagOS] ShagOS file system",
	/* AF */ "[Mac OS X / ShagOS] HFS and HFS+ / ShagOS swap",
	/* B0 */ "[Boot-Star] Boot-Star dummy partition",
	/* B1 */ "[HP Volume Expansion / QNX 6.x] ? / QNX Neutrino power-safe file system",
	/* B2 */ "[QNX 6.x] QNX Neutrino power-safe file system",
	/* B3 */ "[HP Volume Expansion / QNX 6.x] ? / QNX Neutrino power-safe file system",
	/* B4 */ "[HP Volume Expansion] ?",
	/* B5 */ "",
	/* B6 */ "[HP Volume Expansion / Windows NT 4 Server] ? / Corrupted fault-tolerant FAT16B mirrored master volume",
	/* B7 */ "[BSDI (before 3.0) / Windows NT 4 Server] BSDI native file system / swap / Corrupted fault-tolerant HPFS/NTFS mirrored master volume",
	/* B8 */ "[BSDI (before 3.0)] BSDI swap / native file system",
	/* B9 */ "",
	/* BA */ "",
	/* BB */ "[BootWizard, OS Selector / Acronis True Image / Windows NT 4 Server] PTS BootWizard 4 / OS Selector 5 for hidden partitions / OEM Secure Zone / Corrupted fault-tolerant FAT32 mirrored master volume",
	/* BC */ "[Windows NT 4 Server / Acronis True Image / Backup Capsule] Corrupted fault-tolerant FAT32 mirrored master volume / Acronis Secure Zone / Backup Capsule",
	/* BD */ "[BonnyDOS/286] ?",
	/* BE */ "[Solaris 8] Solaris 8 boot",
	/* BF */ "[Solaris] Solaris x86 (for Sun disklabels, since 2005)",
	/* C0 */ "[DR-DOS, Multiuser DOS, REAL/32] Secured FAT partition (smaller than 32 MB)",
	/* C1 */ "[DR DOS 6.0+] Secured FAT12",
	/* C2 */ "[Power Boot] Hidden Linux native file system",
	/* C3 */ "[Power Boot] Hidden Linux swap",
	/* C4 */ "[DR DOS 6.0+] Secured FAT16",
	/* C5 */ "[DR DOS 6.0+] Secured extended partition with CHS addressing",
	/* C6 */ "[DR DOS 6.0+ / Windows NT 4 Server] Secured FAT16B / Corrupted fault-tolerant FAT16B mirrored slave volume",
	/* C7 */ "[Syrinx / Windows NT 4 Server] Syrinx boot / Corrupted fault-tolerant HPFS/NTFS mirrored slave volume",
	/* C8 */ "[?] Reserved for DR-DOS since 1997",
	/* C9 */ "[?] Reserved for DR-DOS since 1997",
	/* CA */ "[?] Reserved for DR-DOS since 1997",
	/* CB */ "[DR-DOS 7.0x / Windows NT 4 Server] Secured FAT32 / Corrupted fault-tolerant FAT32 mirrored slave volume",
	/* CC */ "[DR-DOS 7.0x / Windows NT 4 Server] Secured FAT32 / Corrupted fault-tolerant FAT32 mirrored slave volume",
	/* CD */ "[CTOS / Linux] Memory dump / openSUSE ISOHybrid ISO9660 partition (from openSUSE Leap \"Live\" x86 images)",
	/* CE */ "[DR-DOS 7.0x] Secured FAT16B",
	/* CF */ "[DR-DOS 7.0x] Secured extended partition with LBA",
	/* D0 */ "[Multiuser DOS, REAL/32] Secured FAT partition (larger than 32 MB)",
	/* D1 */ "[Multiuser DOS] Secured FAT12",
	/* D2 */ "",
	/* D3 */ "",
	/* D4 */ "[Multiuser DOS] Secured FAT16",
	/* D5 */ "[Multiuser DOS] Secured extended partition with CHS addressing",
	/* D6 */ "[Multiuser DOS] Secured FAT16B",
	/* D7 */ "",
	/* D8 */ "[CP/M-86] CP/M-86",
	/* D9 */ "",
	/* DA */ "[? / Powercopy Backup] Non-file system data / Shielded disk",
	/* DB */ "[CP/M-86, Concurrent CP/M-86, Concurrent DOS / CTOS / D800 / DRMK] ? / boot image for x86 supervisor CPU (SCPU) module / FAT32 system restore partition (DSR)",
	/* DC */ "",
	/* DD */ "[CTOS] Hidden memory dump",
	/* DE */ "[?] FAT16 utility/diagnostic partition",
	/* DF */ "[DG/UX / BootIt / Aviion] DG/UX virtual disk manager / EMBRM / ?",
	/* E0 */ "[?] ST AVFS",
	/* E1 */ "[SpeedStor] FAT12 (<=16 MB)",
	/* E2 */ "",
	/* E3 */ "[SpeedStor] Read-only FAT12",
	/* E4 */ "[SpeedStor] FAT16 (<=32 MB)",
	/* E5 */ "[Tandy MS-DOS] Logical sectored FAT12 or FAT16",
	/* E6 */ "[SpeedStor] Read-only FAT16",
	/* E7 */ "",
	/* E8 */ "[LUKS] Linux Unified Key Setup",
	/* E9 */ "",
	/* EA */ "",
	/* EB */ "[BeOS, Haiku] BFS",
	/* EC */ "[SkyOS] SkyFS",
	/* ED */ "[EFI] Was proposed for GPT hybrid MBR",
	/* EE */ "[EFI] GPT protective MBR",
	/* EF */ "[EFI] EFI system partition. Can be a FAT12, FAT16, FAT32 (or other) file system",
	/* F0 */ "[Linux] PA-RISC Linux boot loader; must reside in first physical 2 GB",
	/* F1 */ "",
	/* F2 */ "[Sperry IT MS-DOS 3.x, Unisys MS-DOS 3.3, Digital Research DOS Plus 2.1] Logical sectored FAT12 or FAT16 secondary partition",
	/* F3 */ "",
	/* F4 */ "[SpeedStor / Prologue] FAT16B / Single volume partition for NGF or TwinFS",
	/* F5 */ "[Prologue] MD0-MD9 multi volume partition for NGF or TwinFS",
	/* F6 */ "[SpeedStor] Read-only FAT16B",
	/* F7 */ "[O.S.G. / X1] EFAT / Solid State file system",
	/* F8 */ "[Arm EBBR 1.0] Protective partition for the area containing system firmware",
	/* F9 */ "[Linux] pCache ext2/ext3 persistent cache",
	/* FA */ "",
	/* FB */ "[VMware ESX] VMware VMFS file system partition",
	/* FC */ "[VMware ESX] VMware swap / VMKCORE kernel dump partition",
	/* FD */ "[Linux] Linux RAID superblock with auto-detect",
	/* FE */ "[PS/2 / Linux] PS/2 IML partition / PS/2 recovery partition (FAT12 reference disk floppy image) / Old Linux LVM",
	/* FF */ "[XENIX] XENIX bad block table"
};
