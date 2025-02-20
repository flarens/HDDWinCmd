#pragma once

// âåðñèþ òàêæå îáíîâëÿòü â version.rc

static const wchar_t helpText[] = LR"(======================
=== HDDWinCmd v0.1 ===
======================

[Command structure:]

--command parameter1 parameter2 ... --command "parameter one" ...

[Hints:]

- Execution is from left to right.
- If a parameter contains spaces, it must be wrapped with double quotes.
- If a command requires additional parameters, they must be executed before it (left).

[Ñommands:]

--help (or --h or uncommanded run) - command list (this page)

--disk [path] (or --d) - disk path (permanent / temporary Windows / temporary Linux / volume GUID / drive letter / physical drive number)
    examples:
     --d \\?\scsi#disk&ven_hgst&prod_hus726060al5214#5&a26c2b1&0&000000#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}
     --disk 0
     --d /dev/sda
     --d C

--timeout [milliseconds] (or --t) - waiting time for the event required to continue various commands
    default value: 20000
    examples:
     --t 10000 (that means 10 seconds)

--info (or --i) - display information about the disks
--infomin (or --im) - display minimal information about the disks
    requires:
     --disk (optional) - information only for the selected disk
    examples:
     --info --nc
     --d E --i --d /dev/sdb --im

--spin [state] (or --s) - stop/awaken the disk spindle
    requires:
     --disk - destination disk
     --timeout (optional) - waiting time for disk operations to complete
    states:
     0 (or spindown) - stop spindle (used --timeout)
     1 (or spinup) - wake up the disk
    examples:
     --d 1 --s 0
    commentary:
     You cannot stop a disk that is currently undergoing read/write operations.

--plug [state] [flags] (or --p) - disconnect/connect disk to the system (not stop)
    requires:
     --disk - destination disk
     --timeout (optional) - waiting time for disk operations to complete (used if “force” flag is not set)
    states:
     0 (or offline) - go offline
     1 (or online) - go online
    flags (optional):
     duration:
      reboot (default) - valid until system reboot
      permanent - is applied until changed by the command
     safety:
      safe (default) - wait for read/write operations to complete (used --timeout)
      force - forced, even if the disk is in use (Caution! Data loss is possible)
    examples:
     --d \\.\PhysicalDrive1 --t 30000 --p offline permanent safe
     --d 1 --p offline safe --p offline force
     (if you want to force a disk offline after the safe timeout expires, repeat the command with the required flag)
    commentary:
     The program will protect your operating system disk from being taken offline.
     Putting the disk offline will not save it from spinning up during computer startup.

--wait [milliseconds] (or --w) - wait a fixed time before continuing
    examples:
     --d 1 --p offline --w 1000 --s spindown (will wait 1 second between disconnecting the disk and stopping it)

--notclose (or --nc) - do not close the console at the end of execution

--response [variant] (or --r) - specify the console response method
    variants:
     text (default) - standard output of text data to the console
     bin - each command will only give a short answer (success/failure)
     code - text is not output
     hide - hide the console window. In this mode, the --notclose command is ignored
    commentary:
     Normally this command should go first to affect the entire list of commands.
     In modes [code] or [hide] if all commands completed without errors, the application will close with code 0, else 1.
)";