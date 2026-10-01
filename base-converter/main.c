/*
 * ============================================================================
 *  BASE CONVERTER
 *  A menu-driven number system converter written in C.
 *
 *  - Converts whole numbers between any two bases (2 to 36)
 *  - Quick mode for Binary / Octal / Decimal / Hexadecimal
 *  - Shows step-by-step working (repeated division)
 *  - Keeps a history of every conversion in a text file
 *
 *  Build:  gcc -std=c99 -Wall -Wextra -o base_converter main.c
 *  Run:    ./base_converter        (Windows: base_converter.exe)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

/* ---------------------------------------------------------------------------
 *  Constants
 * ------------------------------------------------------------------------ */
#define HISTORY_FILE "conversion_history.log"

#define BOX_INNER 76 /* characters between the two vertical borders   */
#define LABEL_W 16   /* width of the label column inside a result row */
#define VALUE_W 56   /* width of the value column inside a result row */
#define SHOW_LAST 15 /* history rows shown on screen                  */

/* Return codes of parse_number() */
#define PARSE_OK 0
#define PARSE_EMPTY 1
#define PARSE_BAD_DIGIT 2
#define PARSE_TOO_BIG 3

/* The 12 pairs offered in Quick Conversion */
#define QUICK_COUNT 12
int quick_from[QUICK_COUNT] = {2, 2, 2, 8, 8, 8, 10, 10, 10, 16, 16, 16};
int quick_to[QUICK_COUNT] = {8, 10, 16, 2, 10, 16, 2, 8, 16, 2, 8, 10};

/* ---------------------------------------------------------------------------
 *  Function prototypes
 * ------------------------------------------------------------------------ */
void show_goodbye(void);

/* ---------------------------------------------------------------------------
 *  SECTION 1: Small text helpers
 * ------------------------------------------------------------------------ */

/* Clears the console window. */
void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Value of one digit character: '7' -> 7, 'f' -> 15, anything else -> -1 */
int digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    return -1;
}

/* Digit character for a value: 11 -> 'B' */
char digit_char(int value)
{
    if (value < 10)
        return (char)('0' + value);
    return (char)('A' + value - 10);
}

/* Name used in menus. */
const char *base_word(int base)
{
    if (base == 2)
        return "Binary";
    if (base == 8)
        return "Octal";
    if (base == 10)
        return "Decimal";
    if (base == 16)
        return "Hexadecimal";
    return "Custom";
}

/* "Binary (Base 2)" for the four common bases, "Base 7" for the others. */
void base_label(int base, char *out)
{
    if (base == 2 || base == 8 || base == 10 || base == 16)
        sprintf(out, "%s (Base %d)", base_word(base), base);
    else
        sprintf(out, "Base %d", base);
}

/* 1 if the text contains only spaces/tabs (or nothing). */
int is_blank(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return *s == '\0';
}

/* ---------------------------------------------------------------------------
 *  SECTION 2: Input helpers
 * ------------------------------------------------------------------------ */

/* Prints a prompt and reads one line (without the newline).
   If the input stream ends (Ctrl+D / Ctrl+Z), the program exits cleanly. */
void read_line(const char *prompt, char *buf, int size)
{
    int len, c;

    printf("  %s\n  > ", prompt);
    fflush(stdout);

    if (fgets(buf, size, stdin) == NULL)
    {
        printf("\n");
        show_goodbye();
        exit(0);
    }

    len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[--len] = '\0';
    }
    else
    {
        while ((c = getchar()) != '\n' && c != EOF)
        {
        } /* drop the rest */
    }
    if (len > 0 && buf[len - 1] == '\r')
        buf[len - 1] = '\0';
}

/* Keeps asking until the user types a whole number between min and max. */
int read_int_in_range(const char *prompt, int min, int max)
{
    char line[64], *end;
    long v;

    for (;;)
    {
        read_line(prompt, line, sizeof line);
        v = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;
        if (end != line && *end == '\0' && v >= min && v <= max)
            return (int)v;
        printf("\n  [!] Please enter a number between %d and %d.\n\n", min, max);
    }
}

/* Returns 1 for yes, 0 for no. */
int ask_yes_no(const char *question)
{
    char line[16], full[128];

    sprintf(full, "%s (y/n)", question);
    read_line(full, line, sizeof line);
    return line[0] == 'y' || line[0] == 'Y';
}

void pause_screen(void)
{
    char line[16];
    read_line("Press Enter to continue...", line, sizeof line);
}

/* ---------------------------------------------------------------------------
 *  SECTION 3: Conversion logic
 * ------------------------------------------------------------------------ */

/*
 * Reads `text` as a number written in `base`.
 * Accepts an optional sign, an optional 0b / 0o / 0x prefix, and ignores
 * spaces, commas and underscores used to separate digits.
 *
 * On success the number is stored in *value (magnitude) and *neg (1 if the
 * number was negative). Returns one of the PARSE_ codes.
 * If a bad digit is found, it is stored in *bad.
 */
int parse_number(const char *text, int base, int *neg,
                 unsigned long long *value, char *bad)
{
    unsigned long long v = 0;
    int seen = 0, i, d;
    char c, p;

    *neg = 0;
    *value = 0;
    *bad = '\0';

    while (isspace((unsigned char)*text))
        text++;

    if (*text == '-')
    {
        *neg = 1;
        text++;
    }
    else if (*text == '+')
    {
        text++;
    }

    if (text[0] == '0')
    {
        p = (char)tolower((unsigned char)text[1]);
        if ((base == 16 && p == 'x') || (base == 8 && p == 'o') ||
            (base == 2 && p == 'b'))
            text += 2;
    }

    for (i = 0; text[i] != '\0'; i++)
    {
        c = text[i];
        if (c == ' ' || c == '\t' || c == '_' || c == ',')
            continue;

        d = digit_value(c);
        if (d < 0 || d >= base)
        {
            *bad = c;
            return PARSE_BAD_DIGIT;
        }
        if (v > (ULLONG_MAX - d) / base)
            return PARSE_TOO_BIG; /* overflow check */
        v = v * base + d;
        seen = 1;
    }

    if (!seen)
        return PARSE_EMPTY;

    *value = v;
    if (v == 0)
        *neg = 0; /* "-0" is just 0 */
    return PARSE_OK;
}

/* Writes the number into `out` as text in the given base. */
void to_base(unsigned long long v, int neg, int base, char *out)
{
    char tmp[72];
    int n = 0, i = 0;

    if (v == 0)
        tmp[n++] = '0';
    while (v > 0)
    {
        tmp[n++] = digit_char((int)(v % base));
        v /= base;
    }

    if (neg)
        out[i++] = '-';
    while (n > 0)
        out[i++] = tmp[--n]; /* digits were produced backwards */
    out[i] = '\0';
}

/* How many bits are needed to store v (at least 1). */
int bit_count(unsigned long long v)
{
    int bits = 0;
    do
    {
        bits++;
        v /= 2;
    } while (v > 0);
    return bits;
}

/* ---------------------------------------------------------------------------
 *  SECTION 4: Drawing helpers (boxes and rows)
 * ------------------------------------------------------------------------ */

void print_line(void)
{
    int i;
    printf("+");
    for (i = 0; i < BOX_INNER; i++)
        putchar('-');
    printf("+\n");
}

void print_center(const char *text)
{
    int len = (int)strlen(text);
    int left = (BOX_INNER - len) / 2;
    int right = BOX_INNER - len - left;
    printf("|%*s%s%*s|\n", left, "", text, right, "");
}

void print_text(const char *text)
{
    printf("| %-*s |\n", BOX_INNER - 2, text);
}

/* One menu line:  [1]  Title                 short description */
void print_menu_item(const char *key, const char *title, const char *desc)
{
    printf("|  %-5s %-26s%-41s |\n", key, title, desc);
}

/* "Label : value" row. Long values wrap onto extra lines (at spaces if possible). */
void print_row(const char *label, const char *value)
{
    int len = (int)strlen(value), pos = 0, first = 1, take, k;

    do
    {
        take = len - pos;
        if (take > VALUE_W)
        {
            take = VALUE_W;
            k = VALUE_W;
            while (k > 0 && value[pos + k] != ' ')
                k--;
            if (k > 0)
                take = k;
        }

        printf("| %-*s%s%-*.*s |\n", LABEL_W, first ? label : "",
               first ? ": " : "  ", VALUE_W, take, value + pos);

        pos += take;
        while (value[pos] == ' ')
            pos++;
        first = 0;
    } while (pos < len);
}

/* Inserts separators for readability: 1111 0000 / 1,000,000 / 7FFF FFFF */
void group_digits(const char *num, int base, char *out)
{
    int size = 0, pos = 0, i, len;
    char sep = ' ';

    if (*num == '-')
    {
        out[pos++] = '-';
        num++;
    }

    if (base == 2 || base == 16)
        size = 4;
    else if (base == 8)
        size = 3;
    else if (base == 10)
    {
        size = 3;
        sep = ',';
    }

    len = (int)strlen(num);
    for (i = 0; i < len; i++)
    {
        if (size > 0 && i > 0 && (len - i) % size == 0)
            out[pos++] = sep;
        out[pos++] = num[i];
    }
    out[pos] = '\0';
}

/* ---------------------------------------------------------------------------
 *  SECTION 5: Screens
 * ------------------------------------------------------------------------ */

void show_banner(void)
{
    print_line();
    print_center("");
    print_center("B A S E   C O N V E R T E R");
    print_center("Number System Conversion Toolkit");
    print_center("");
    print_center("Binary  |  Octal  |  Decimal  |  Hexadecimal  |  Bases 2-36");
    print_center("");
    print_line();
}

void show_welcome(void)
{
    clear_screen();
    show_banner();
    printf("\n");
    print_line();
    print_center("WELCOME");
    print_line();
    print_text("Convert whole numbers between any two bases (2 to 36).");
    print_text("Every conversion is saved to a log file for later review.");
    print_text("Tip: leave an input blank at any prompt to go back.");
    print_line();
    printf("\n");
    pause_screen();
}

void show_goodbye(void)
{
    printf("\n");
    print_line();
    print_center("");
    print_center("Thank you for using Base Converter. Goodbye!");
    print_center("");
    print_line();
    printf("\n");
}

void show_main_menu(void)
{
    printf("\n");
    print_line();
    print_center("MAIN MENU");
    print_line();
    print_menu_item("[1]", "Quick Conversion", "Pick from 12 common base pairs");
    print_menu_item("[2]", "Custom Conversion", "Any base from 2 to 36");
    print_menu_item("[3]", "Convert to All Bases", "Bin / Oct / Dec / Hex at once");
    print_menu_item("[4]", "View History", "Recent conversions from the log");
    print_menu_item("[5]", "Clear History", "Erase the log file");
    print_menu_item("[6]", "Help & About", "Supported formats and limits");
    print_menu_item("[0]", "Exit", "Close the program");
    print_line();
    printf("\n");
}

void show_quick_menu(void)
{
    int i;
    char key[16], title[64];

    printf("\n");
    print_line();
    print_center("QUICK CONVERSION");
    print_line();
    for (i = 0; i < QUICK_COUNT; i++)
    {
        sprintf(key, "[%d]", i + 1);
        sprintf(title, "%s -> %s", base_word(quick_from[i]), base_word(quick_to[i]));
        print_menu_item(key, title, "");
    }
    print_menu_item("[0]", "Back", "Return to the main menu");
    print_line();
    printf("\n");
}

void show_help(void)
{
    printf("\n");
    print_line();
    print_center("HELP & ABOUT");
    print_line();
    print_row("Supported bases", "2 to 36 (digits 0-9 then letters A-Z)");
    print_row("Prefixes", "0b (binary), 0o (octal), 0x (hex) are accepted");
    print_row("Separators", "spaces, commas and underscores are ignored");
    print_row("Largest value", "18,446,744,073,709,551,615 (64-bit)");
    print_row("Negatives", "sign-magnitude, for example -FF");
    print_row("Fractions", "not supported (whole numbers only)");
    print_row("Log file", HISTORY_FILE " (current folder)");
    print_line();
    printf("\n");
}

void print_section(const char *title)
{
    int i, len = (int)strlen(title);

    printf("\n  %s\n  ", title);
    for (i = 0; i < len; i++)
        putchar('-');
    printf("\n\n");
}

void show_parse_error(int status, char bad, int base)
{
    if (status == PARSE_BAD_DIGIT)
        printf("\n  [!] '%c' is not a valid digit in base %d.\n\n", bad, base);
    else if (status == PARSE_TOO_BIG)
        printf("\n  [!] Number too large (maximum is 18,446,744,073,709,551,615).\n\n");
    else
        printf("\n  [!] No digits were entered.\n\n");
}

/* ---------------------------------------------------------------------------
 *  SECTION 6: Result displays
 * ------------------------------------------------------------------------ */

void show_result_card(const char *input, int from, int to, const char *result,
                      unsigned long long value, int neg)
{
    char from_txt[40], to_txt[40], gin[200], gout[200], bits[32];

    base_label(from, from_txt);
    base_label(to, to_txt);
    group_digits(input, from, gin);
    group_digits(result, to, gout);
    sprintf(bits, "%d bit(s)", bit_count(value));

    printf("\n");
    print_line();
    print_center("CONVERSION RESULT");
    print_line();
    print_row("From", from_txt);
    print_row("To", to_txt);
    print_line();
    print_row("Input", gin);
    print_row("Result", gout);
    if (strcmp(gout, result) != 0)
        print_row("Result (plain)", result);
    print_row("Magnitude bits", bits);
    print_line();
    if (neg)
    {
        print_text("Note: negative values use sign-magnitude notation.");
        print_line();
    }
    printf("\n");
}

void show_all_bases_card(const char *input, int from, unsigned long long value, int neg)
{
    int targets[4] = {2, 8, 10, 16};
    const char *labels[4] = {"Binary (2)", "Octal (8)", "Decimal (10)", "Hex (16)"};
    char src[40], gin[200], out[100], gout[200], bits[32];
    int i;

    base_label(from, src);
    group_digits(input, from, gin);
    sprintf(bits, "%d bit(s)", bit_count(value));

    printf("\n");
    print_line();
    print_center("ALL COMMON BASES");
    print_line();
    print_row("Source base", src);
    print_row("Input", gin);
    print_row("Magnitude bits", bits);
    print_line();
    for (i = 0; i < 4; i++)
    {
        to_base(value, neg, targets[i], out);
        group_digits(out, targets[i], gout);
        print_row(labels[i], gout);
    }
    print_line();
    printf("\n");
}

void print_steps_border(void)
{
    printf("  +------+----------------------------+----------------------+------------+\n");
}

/* Shows the repeated-division method used to convert into `base`. */
void show_steps(unsigned long long value, int base)
{
    unsigned long long dividend[64], quotient[64];
    int remainder[64];
    char digits[72], division[48], rem_txt[24];
    int n = 0, i;

    printf("\n  Step-by-step working (repeated division by %d)\n\n", base);

    if (value == 0)
    {
        printf("  0 is written as 0 in every base.\n\n");
        return;
    }

    while (value > 0 && n < 64)
    {
        dividend[n] = value;
        quotient[n] = value / base;
        remainder[n] = (int)(value % base);
        value = quotient[n];
        n++;
    }

    print_steps_border();
    printf("  | %-4s | %-26s | %-20s | %-10s |\n", "Step", "Division", "Quotient", "Remainder");
    print_steps_border();
    for (i = 0; i < n; i++)
    {
        sprintf(division, "%llu / %d", dividend[i], base);
        sprintf(rem_txt, "%d (%c)", remainder[i], digit_char(remainder[i]));
        printf("  | %-4d | %-26s | %-20llu | %-10s |\n",
               i + 1, division, quotient[i], rem_txt);
        digits[n - 1 - i] = digit_char(remainder[i]); /* last remainder = first digit */
    }
    digits[n] = '\0';
    print_steps_border();

    printf("\n  Read the remainders from bottom to top:  %s\n\n", digits);
}

/* ---------------------------------------------------------------------------
 *  SECTION 7: History file
 * ------------------------------------------------------------------------ */

/* Adds one line to the log:  date time|input|from|to|result */
void save_history(const char *input, int from, int to, const char *result)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char stamp[32];
    FILE *fp;

    strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", t);

    fp = fopen(HISTORY_FILE, "a");
    if (fp == NULL)
    {
        printf("  [!] Could not write to %s\n", HISTORY_FILE);
        return;
    }
    fprintf(fp, "%s|%s|%d|%d|%s\n", stamp, input, from, to, result);
    fclose(fp);
}

/* Copies s into out; if longer than width, ends it with ".." */
void shorten(const char *s, int width, char *out)
{
    int len = (int)strlen(s);

    if (len <= width)
    {
        strcpy(out, s);
    }
    else
    {
        strncpy(out, s, width - 2);
        out[width - 2] = '.';
        out[width - 1] = '.';
        out[width] = '\0';
    }
}

void print_history_border(void)
{
    printf("+-----+------------------+------------------+------+------+------------------+\n");
}

void view_history(void)
{
    FILE *fp = fopen(HISTORY_FILE, "r");
    char line[512], stamp[32], in[100], res[100], in_short[32], res_short[32];
    int total = 0, skip, index = 0, shown = 0, from, to;

    print_section("Conversion History");

    if (fp == NULL)
    {
        printf("  [i] No history yet. Run a conversion and it will appear here.\n\n");
        return;
    }

    while (fgets(line, sizeof line, fp) != NULL)
        total++;
    if (total == 0)
    {
        fclose(fp);
        printf("  [i] No history yet. Run a conversion and it will appear here.\n\n");
        return;
    }

    rewind(fp);
    skip = (total > SHOW_LAST) ? total - SHOW_LAST : 0;

    print_history_border();
    printf("| %-3s | %-16s | %-16s | %-4s | %-4s | %-16s |\n",
           "#", "Date & Time", "Input", "From", "To", "Result");
    print_history_border();

    while (fgets(line, sizeof line, fp) != NULL)
    {
        index++;
        if (index <= skip)
            continue;
        if (sscanf(line, "%19[^|]|%80[^|]|%d|%d|%80[^\n]",
                   stamp, in, &from, &to, res) != 5)
            continue;

        shorten(in, 16, in_short);
        shorten(res, 16, res_short);
        printf("| %3d | %-16.16s | %-16s | %4d | %4d | %-16s |\n",
               index, stamp, in_short, from, to, res_short);
        shown++;
    }
    print_history_border();
    fclose(fp);

    printf("\n  Showing the last %d of %d conversion(s). Full records: %s\n\n",
           shown, total, HISTORY_FILE);
}

/* Returns 1 if the history is now empty/gone, 0 if the file could not be removed. */
int clear_history(void)
{
    FILE *fp;

    remove(HISTORY_FILE);
    fp = fopen(HISTORY_FILE, "r");
    if (fp != NULL)
    {
        fclose(fp);
        return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 *  SECTION 8: Conversion workflows
 * ------------------------------------------------------------------------ */

/* Asks for a number in `from` until it is valid.
   Returns 1 when a number was read, 0 if the user left the line blank. */
int get_number(int from, char *normalized, unsigned long long *value, int *neg)
{
    char input[128], prompt[80], bad;
    int status;

    if (from <= 10)
        printf("  [i] Valid digits for base %d: 0-%d\n", from, from - 1);
    else
        printf("  [i] Valid digits for base %d: 0-9 and A-%c\n", from, digit_char(from - 1));
    printf("  [i] Leave blank to go back.\n\n");
    sprintf(prompt, "Enter the %s number", base_word(from));

    for (;;)
    {
        read_line(prompt, input, sizeof input);
        if (is_blank(input))
            return 0;

        status = parse_number(input, from, neg, value, &bad);
        if (status == PARSE_OK)
        {
            to_base(*value, *neg, from, normalized); /* cleaned-up version of the input */
            return 1;
        }
        show_parse_error(status, bad, from);
    }
}

/* Repeated conversions between one fixed pair of bases. */
void convert_session(int from, int to)
{
    char title[100], from_txt[40], to_txt[40], normalized[100], result[100];
    unsigned long long value;
    int neg;

    base_label(from, from_txt);
    base_label(to, to_txt);
    sprintf(title, "Convert %s to %s", from_txt, to_txt);
    print_section(title);

    while (get_number(from, normalized, &value, &neg))
    {
        to_base(value, neg, to, result);
        show_result_card(normalized, from, to, result, value, neg);
        save_history(normalized, from, to, result);

        if (ask_yes_no("Show step-by-step working?"))
            show_steps(value, to);
        if (!ask_yes_no("Convert another number with the same bases?"))
            return;
        printf("\n");
    }
}

void quick_conversion(void)
{
    int choice;

    clear_screen();
    show_banner();
    show_quick_menu();
    choice = read_int_in_range("Select a conversion", 0, QUICK_COUNT);
    if (choice == 0)
        return;
    convert_session(quick_from[choice - 1], quick_to[choice - 1]);
}

void custom_conversion(void)
{
    int from, to;

    clear_screen();
    show_banner();
    print_section("Custom Conversion");
    from = read_int_in_range("Source base (2-36)", 2, 36);
    to = read_int_in_range("Target base (2-36)", 2, 36);
    if (from == to)
        printf("  [i] Source and target are the same base; the number will not change.\n");
    convert_session(from, to);
}

void all_bases_conversion(void)
{
    int targets[4] = {2, 8, 10, 16};
    char normalized[100], result[100];
    unsigned long long value;
    int from, neg, i;

    clear_screen();
    show_banner();
    print_section("Convert to All Common Bases");
    from = read_int_in_range("Source base (2-36)", 2, 36);
    printf("\n");

    if (!get_number(from, normalized, &value, &neg))
        return;

    show_all_bases_card(normalized, from, value, neg);
    for (i = 0; i < 4; i++)
    {
        if (targets[i] == from)
            continue;
        to_base(value, neg, targets[i], result);
        save_history(normalized, from, targets[i], result);
    }
    pause_screen();
}

/* ---------------------------------------------------------------------------
 *  SECTION 9: Main program
 * ------------------------------------------------------------------------ */

int main(void)
{
    int choice;

    show_welcome();

    for (;;)
    {
        clear_screen();
        show_banner();
        show_main_menu();
        choice = read_int_in_range("Select an option [0-6]", 0, 6);

        switch (choice)
        {
        case 1:
            quick_conversion();
            break;
        case 2:
            custom_conversion();
            break;
        case 3:
            all_bases_conversion();
            break;
        case 4:
            clear_screen();
            show_banner();
            view_history();
            pause_screen();
            break;
        case 5:
            if (ask_yes_no("Erase all saved history?"))
            {
                if (clear_history())
                    printf("\n  [OK] History cleared.\n\n");
                else
                    printf("\n  [!] Could not delete the history file.\n\n");
            }
            pause_screen();
            break;
        case 6:
            clear_screen();
            show_banner();
            show_help();
            pause_screen();
            break;
        case 0:
            show_goodbye();
            return 0;
        }
    }
}
