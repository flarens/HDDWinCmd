# HDDWinCmd

**Платформа:** Windows

**Интерфейс:** Консольное приложение (English)

**Требования:** Запуск от имени администратора *(если учетная запись администратора не запаролена, дополнительных действий при запуске не требуется)*

## Описание:

Программа предоставляет простой интерфейс для управления жесткими дисками компьютера. Среди ключевых функций — программное отключение и подключение дисков в операционной системе, аналогичное команде `offline disk` в `diskpart` или отключению дисков через `diskmgmt.msc`. Также реализована возможность остановки шпинделя дисков. Программа позволяет отключать USB-накопители, что недоступно в большинстве аналогичных решений.
В некоторых случаях утилита способна усыплять диски, даже если в драйвере контроллера (например, LSI HBA) отключена возможность управления питанием. Для обеспечения безопасности предусмотрены защитные механизмы: перед отключением диска выполняется проверка на наличие системного тома и активных операций чтения/записи.

## Программа предоставляет информацию о дисках:

**Информация о дисковом устройстве:** путь к диску, тип шины, имя продукта и поставщика, версия, серийный номер, возможность извлечения, поддержка NCQ, геометрия диска, размер, тип таблицы разделов

**Информация о логическом томе:** GPT GUID, volume GUID, буква диска, метка тома, файловая система, start LBA, размер, тип и имя партиции, GPT атрибуты

> Рекомендуется в командах использовать путь к диску `permanent device path`, он сохранится при изменении конфигурации оборудования

![screenshot](https://github.com/flarens/HDDWinCmd/blob/main/screenshot.png)

## Структура команд:

    --command parameter1 parameter2 ... --command "parameter one" ...

## Подсказки:

- Порядок выполнения командной строки - слева направо.
- Если параметр содержит пробелы, он должен быть заключен в двойные кавычки.
- Если команде требуются дополнительные параметры, они должны быть заданы ранее (левее).

## Список команд:

---

`--help` (или `--h` или запуск без команд) - **показать список всех команд**

---

`--disk [path]` (или `--d`) - **задать путь к диску** (permanent / temporary Windows / temporary Linux / volume GUID / drive letter / physical drive number)

> *примеры:*

    --d \\?\scsi#disk&ven_hgst&prod_hus726060al5214#5&a26c2b1&0&000000#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}
    --disk 0
    --d /dev/sda
    --d C

---

`--timeout [milliseconds]` (или `--t`) - **время ожидания благоприятных условий для выполнения некоторых команд**

> *значение по умолчанию:*

20000

> *примеры:*

`--t 10000` (эквивалент 10 секундам)

---

`--info` (или `--i`) - **отобразить информацию о дисках**

`--infomin` (или `--im`) - **отобразить минимальную информацию о дисках**

> *требуется:*

`--disk` (опционально) - информация только о выбранном диске

> *примеры:*

     --info --nc
     --d E --i --d /dev/sdb --im

---

`--spin [state]` (или `--s`) - **остановить/разбудить диск**

> *требуется:*

`--disk` - целевой диск

`--timeout` (опционально) - время ожидания завершения дисковых операций

> *состояния:*

`0` (или `spindown`) - остановка шпинделя (использует `--timeout`)

`1` (или `spinup`) - пробуждение диска

> *примеры:*

    --d 1 --s 0

> *комментарий:*

Нельзя остановить диск, на котором в данный момент выполняются операции чтения/записи.

---

`--plug [state] [flags]` (или `--p`) - **отключить/подключить диск к системе** (не остановить)

> *требуется:*

`--disk` - целевой диск

`--timeout` (опционально) - время ожидания завершения дисковых операций (используется, если флаг `force` не установлен)

> *состояния:*

`0` (или `offline`) - отключить

`1` (или `online`) - подключить

> *флаги (опционально):*

*срок действия:*

`reboot` (по умолчанию) - действует до перезагрузки системы

`permanent` - действует до изменения командой

*безопасность:*

`safe` (по умолчанию) - перед отключением ожидает завершения операций чтения/записи (использует `--timeout`)

`force` - принудительно, даже если диск используется (**Внимание!** Есть риск частичной потери данных)

> *примеры:*

    --d \\.\PhysicalDrive1 --t 30000 --p offline permanent safe

Если вы хотите принудительно отключить диск по истечению тайм-аута безопасного отключения, повторите команду с нужным флагом:
    
    --d 1 --p offline safe --p offline force

> *комментарий:*

Программа защищает диск с операционной системой от случайного отключения.

Перевод диска в автономный режим не предотвратит его раскрутку при запуске компьютера.

---

`--wait [milliseconds]` (или `--w`) - **подождать фиксированное время пред продолжением**

> *примеры:*

`--d 1 --p offline --w 1000 --s spindown` (будет ждать 1 секунду между отсоединением диска и его остановкой)

---

`--notclose` (или `--nc`) - **не закрывать консоль по окончании выполнения**

---

`--response [variant]` (или `--r`) - **метод ответа приложения**

> варианты:

`text` (по умолчанию) - стандартный вывод текстовых данных в консоль

`bin` - каждая команда дает только короткий ответ (success/failure)

`code` - текст не выводится

`hide` - скрыть окно консоли. В этом режиме команда `--notclose` игнорируется

>*комментарий:*

Обычно эта команда должна быть первой, чтобы повлиять на весь список команд.

В режимах `code` или `hide`, если все команды выполнены без ошибок, приложение закроется с кодом `0`, иначе `1`.
