# Base Converter

A menu-driven number system converter written in C. It converts whole numbers between any two bases from 2 to 36, shows the step-by-step working, and keeps a persistent history in a log file.

Written in a single `main.c` using plain standard C (loops, arrays, functions and file I/O), with no external libraries.

## Features

- **Quick Conversion** - 12 ready-made pairs between Binary, Octal, Decimal and Hexadecimal
- **Custom Conversion** - any source and target base from 2 to 36
- **Convert to All Bases** - one input, shown in Binary, Octal, Decimal and Hex together
- **Step-by-step working** - the repeated-division table behind every result
- **Persistent history** - every conversion is appended to `conversion_history.log`; view or clear it from the menu
- **Input validation** - rejects invalid digits (and names the bad character), empty input and 64-bit overflow
- **Friendly input** - accepts `0b` / `0o` / `0x` prefixes, negative numbers, and `_`, `,` or spaces as digit separators
- **Readable output** - digits are grouped (`1111 0000`, `1,000,000`) and long values wrap inside the result card
- **Portable UI** - ASCII-only boxes; colour is enabled automatically on terminals that support it

## Demo

Main menu:

```
+----------------------------------------------------------------------------+
|                                 MAIN MENU                                  |
+----------------------------------------------------------------------------+
|  [1]   Quick Conversion          Pick from 12 common base pairs            |
|  [2]   Custom Conversion         Any base from 2 to 36                     |
|  [3]   Convert to All Bases      Bin / Oct / Dec / Hex at once             |
|  [4]   View History              Recent conversions from the log           |
|  [5]   Clear History             Erase the log file                        |
|  [6]   Help & About              Supported formats and limits              |
|  [0]   Exit                      Close the program                         |
+----------------------------------------------------------------------------+
```

Result card with step-by-step working (Decimal to Binary):

```
+----------------------------------------------------------------------------+
|                             CONVERSION RESULT                              |
+----------------------------------------------------------------------------+
| From            : Decimal (Base 10)                                        |
| To              : Binary (Base 2)                                          |
+----------------------------------------------------------------------------+
| Input           : 255                                                      |
| Result          : 1111 1111                                                |
| Result (plain)  : 11111111                                                 |
| Magnitude bits  : 8 bit(s)                                                 |
+----------------------------------------------------------------------------+

  Step-by-step working (repeated division by 2)

  +------+----------------------------+----------------------+------------+
  | Step | Division                   | Quotient             | Remainder  |
  +------+----------------------------+----------------------+------------+
  | 1    | 255 / 2                    | 127                  | 1 (1)      |
  | 2    | 127 / 2                    | 63                   | 1 (1)      |
  | 3    | 63 / 2                     | 31                   | 1 (1)      |
  | 4    | 31 / 2                     | 15                   | 1 (1)      |
  | 5    | 15 / 2                     | 7                    | 1 (1)      |
  | 6    | 7 / 2                      | 3                    | 1 (1)      |
  | 7    | 3 / 2                      | 1                    | 1 (1)      |
  | 8    | 1 / 2                      | 0                    | 1 (1)      |
  +------+----------------------------+----------------------+------------+

  Read the remainders from bottom to top:  11111111
```

Convert to all common bases:

```
+----------------------------------------------------------------------------+
|                              ALL COMMON BASES                              |
+----------------------------------------------------------------------------+
| Source base     : Hexadecimal (Base 16)                                    |
| Input           : FF                                                       |
| Magnitude bits  : 8 bit(s)                                                 |
+----------------------------------------------------------------------------+
| Binary (2)      : 1111 1111                                                |
| Octal (8)       : 377                                                      |
| Decimal (10)    : 255                                                      |
| Hex (16)        : FF                                                       |
+----------------------------------------------------------------------------+
```

History view:

```
+-----+------------------+------------------+------+------+------------------+
| #   | Date & Time      | Input            | From | To   | Result           |
+-----+------------------+------------------+------+------+------------------+
|   1 | 2026-10-01 10:08 | 255              |   10 |    2 | 11111111         |
|   2 | 2026-10-01 10:08 | FF               |   16 |    2 | 11111111         |
+-----+------------------+------------------+------+------+------------------+
```

## Build and run

Everything lives in one file, so a single command is enough.

**Linux / macOS**

```bash
gcc -std=c99 -Wall -Wextra -o base_converter main.c
./base_converter
```

**Windows (MinGW / MSYS2 with gcc)**

```bash
gcc -std=c99 -Wall -Wextra -o base_converter.exe main.c
base_converter.exe
```

Any C99-or-newer compiler works. The program writes `conversion_history.log` in the folder you run it from.

## Project structure

```
base-converter/
├── main.c       # the whole program
├── README.md
├── LICENSE
└── .gitignore
```

`main.c` is divided into commented sections, top to bottom:

| Section | What it holds |
|---------|---------------|
| 1. Text helpers | digit/char conversion, base names, `clear_screen()` |
| 2. Input helpers | `read_line()`, `read_int_in_range()`, `ask_yes_no()` |
| 3. Conversion logic | `parse_number()`, `to_base()`, `bit_count()` |
| 4. Drawing helpers | boxes, centred text, wrapped label/value rows, digit grouping |
| 5. Screens | banner, welcome, menus, help |
| 6. Result displays | result card, all-bases card, step-by-step table |
| 7. History file | save, view and clear `conversion_history.log` |
| 8. Workflows | quick, custom and all-bases conversion |
| 9. Main program | the menu loop |

## How it works

1. `parse_number()` checks the text against the chosen base and builds an `unsigned long long` value plus a sign flag. Overflow is tested before every multiply-add, so oversized input is rejected instead of wrapping around.
2. `to_base()` writes that value in any target base by repeated division, so every conversion goes through one common value.
3. `show_steps()` records each division so the working can be displayed as a table.
4. `save_history()` appends one line per conversion to `conversion_history.log`:

```
2026-10-01 10:08:41|255|10|2|11111111
```

## Limitations

- Whole numbers only (no fractional parts like `10.101`)
- Maximum value is 18,446,744,073,709,551,615 (64-bit unsigned)
- Negative numbers use sign-magnitude notation (`-FF`), not two's complement
- The history log lives in the folder you run the program from

## Ideas for extending it

- Fractional number support
- Two's complement view for negative values
- Batch mode that converts numbers read from a file
- Splitting `main.c` into modules (logic, UI, storage) with unit tests

## License

MIT - see [LICENSE](LICENSE).
