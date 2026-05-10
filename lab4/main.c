/*
 * Lab 4: Двоичные сортированные деревья библиографических источников
 *
 * Программа считывает набор .bib файлов, строит двоичное сортированное дерево
 * записей о книгах, упорядоченных по автору (алфавитно), для одного автора —
 * по названиям книг.
 *
 * Вход: список .bib файлов в аргументах командной строки.
 * Выход: файл "output.txt" в кодировке cp1251 с упорядоченными записями.
 *
 * Кодировка исходных bib-файлов: UTF-8 (определяется автоматически).
 * Для конвертации используется iconv.
 * Для сравнения строк в cp1251 используется собственная реализация strncmp для cp1251.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>
#include <locale.h>
#include <wchar.h>

/* Максимальные размеры полей */
#define MAX_AUTHOR_LEN    512
#define MAX_TITLE_LEN     1024
#define MAX_PUBLISHER_LEN 512
#define MAX_ISBN_LEN      128
#define MAX_YEAR_LEN      32
#define MAX_SERIES_LEN    512
#define MAX_EDITION_LEN   64
#define MAX_VOLUME_LEN    64
#define MAX_URL_LEN       1024
#define MAX_LINE_LEN      4096
#define MAX_BIB_FIELDS    20
#define MAX_FILENAME_LEN  1024

/* Количество символов для сравнения по первым N символам */
#define PREFIX_CMP_LEN    5

/* Внутренняя кодировка для хранения и сравнения — cp1251 */
/* Все поля хранятся в cp1251 после конвертации из UTF-8 */

/* ---------------------------------------------------------------
 * Структура записи библиографического источника (BibRecord)
 * ---------------------------------------------------------------
 * Хранит все поля из bib-записи в кодировке cp1251.
 */
typedef struct BibRecord {
    char author[MAX_AUTHOR_LEN];
    char title[MAX_TITLE_LEN];
    char publisher[MAX_PUBLISHER_LEN];
    char isbn[MAX_ISBN_LEN];
    char year[MAX_YEAR_LEN];
    char series[MAX_SERIES_LEN];
    char edition[MAX_EDITION_LEN];
    char volume[MAX_VOLUME_LEN];
    char url[MAX_URL_LEN];
} BibRecord;

/* ---------------------------------------------------------------
 * Структура узла двоичного сортированного дерева (TreeNode)
 * ---------------------------------------------------------------
 * Ключ сортировки: author (основной), title (вторичный).
 * Сравнение — по первым PREFIX_CMP_LEN символам (strncmp_cp1251),
 * при равенстве префиксов — полное сравнение строк.
 */
typedef struct TreeNode {
    BibRecord record;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

/* ---------------------------------------------------------------
 * Утилиты для работы с iconv (UTF-8 -> cp1251)
 * ---------------------------------------------------------------
 */

/**
 * Преобразует строку из UTF-8 в cp1251, используя iconv.
 * @param src Исходная строка в UTF-8.
 * @param dst Буфер для результата в cp1251.
 * @param dst_size Размер буфера dst.
 * @return Указатель на dst при успехе, NULL при ошибке.
 */
static char *utf8_to_cp1251(const char *src, char *dst, size_t dst_size) {
    iconv_t cd;
    char *inbuf = (char *)src;
    size_t inbytesleft = strlen(src);
    char *outbuf = dst;
    size_t outbytesleft = dst_size - 1;

    cd = iconv_open("CP1251", "UTF-8");
    if (cd == (iconv_t)-1) {
        /* Если iconv не работает, копируем как есть (ASCII-совместимость) */
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
        return dst;
    }

    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        /* При ошибке конвертации копируем исходную строку */
        iconv_close(cd);
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
        return dst;
    }

    *outbuf = '\0';
    iconv_close(cd);
    return dst;
}

/* ---------------------------------------------------------------
 * Сравнение строк в cp1251 (аналог strncmp для cp1251)
 * ---------------------------------------------------------------
 *
 * Для cp1251 кириллические символы расположены в диапазоне 0xC0-0xFF.
 * Порядок следования букв в cp1251 совпадает с алфавитным порядком
 * русского алфавита (А-Я, а-я). Это позволяет использовать
 * прямое байтовое сравнение для строк в cp1251.
 *
 * Однако нужно учитывать, что заглавные и строчные буквы должны
 * сравниваться без учёта регистра. Для латиницы используем tolower().
 * Для кириллицы в cp1251: заглавные А-Я = 0xC0-0xDF, строчные а-я = 0xE0-0xFF.
 * Смещение: строчная = заглавная + 0x20.
 */

/**
 * Преобразует символ cp1251 в нижний регистр (только буквы).
 * @param c Байт символа в cp1251.
 * @return Символ в нижнем регистре.
 */
static unsigned char cp1251_tolower(unsigned char c) {
    /* Латинские заглавные A-Z */
    if (c >= 'A' && c <= 'Z')
        return (unsigned char)(c - 'A' + 'a');
    /* Кириллические заглавные А-Я (0xC0-0xDF) */
    if (c >= 0xC0 && c <= 0xDF)
        return (unsigned char)(c + 0x20);
    /* Ё (0xA8 -> 0xB8) */
    if (c == 0xA8)
        return 0xB8;
    return c;
}

/**
 * Сравнение строк в cp1251 без учёта регистра.
 * Аналог strcasecmp для cp1251.
 *
 * @param s1 Первая строка в cp1251.
 * @param s2 Вторая строка в cp1251.
 * @return Отрицательное, ноль или положительное число.
 */
static int strcmp_cp1251(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        unsigned char c1 = cp1251_tolower((unsigned char)*s1);
        unsigned char c2 = cp1251_tolower((unsigned char)*s2);
        if (c1 != c2)
            return (int)c1 - (int)c2;
        s1++;
        s2++;
    }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

/**
 * Сравнение первых n символов строк в cp1251 без учёта регистра.
 * Аналог strncasecmp для cp1251.
 * Собственная реализация для работы с русскими символами в cp1251.
 *
 * @param s1 Первая строка в cp1251.
 * @param s2 Вторая строка в cp1251.
 * @param n Максимальное количество символов для сравнения.
 * @return Отрицательное, ноль или положительное число.
 */
static int strncmp_cp1251(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return 0;

    while (n > 0 && *s1 && *s2) {
        unsigned char c1 = cp1251_tolower((unsigned char)*s1);
        unsigned char c2 = cp1251_tolower((unsigned char)*s2);
        if (c1 != c2)
            return (int)c1 - (int)c2;
        s1++;
        s2++;
        n--;
    }

    if (n == 0)
        return 0;

    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

/**
 * Полное сравнение записей для сортировки в дереве.
 *
 * Правила сортировки:
 * 1. Первичная сортировка — по автору (алфавитно, без учёта регистра).
 * 2. Вторичная сортировка — по названию книги (алфавитно, без учёта регистра).
 *
 * Сначала сравниваются первые PREFIX_CMP_LEN символов поля author.
 * Если префиксы равны — сравниваются полные строки author.
 * Если авторы равны — сравниваются первые PREFIX_CMP_LEN символов title.
 * Если префиксы названий равны — сравниваются полные строки title.
 *
 * @param a Указатель на первую запись.
 * @param b Указатель на вторую запись.
 * @return Отрицательное, если a < b; положительное, если a > b; 0 если равны.
 */
static int compare_records(const BibRecord *a, const BibRecord *b) {
    int cmp;

    /* Сравнение первых PREFIX_CMP_LEN символов author */
    cmp = strncmp_cp1251(a->author, b->author, PREFIX_CMP_LEN);
    if (cmp != 0)
        return cmp;

    /* Полное сравнение author */
    cmp = strcmp_cp1251(a->author, b->author);
    if (cmp != 0)
        return cmp;

    /* Сравнение первых PREFIX_CMP_LEN символов title */
    cmp = strncmp_cp1251(a->title, b->title, PREFIX_CMP_LEN);
    if (cmp != 0)
        return cmp;

    /* Полное сравнение title */
    return strcmp_cp1251(a->title, b->title);
}

/* ---------------------------------------------------------------
 * Работа с двоичным сортированным деревом
 * ---------------------------------------------------------------
 */

/**
 * Создаёт новый узел дерева с заданной записью.
 * @param record Указатель на запись для копирования в узел.
 * @return Указатель на созданный узел или NULL при ошибке.
 */
static TreeNode *tree_node_create(const BibRecord *record) {
    TreeNode *node = (TreeNode *)calloc(1, sizeof(TreeNode));
    if (!node) {
        fprintf(stderr, "Ошибка: не удалось выделить память для узла дерева.\n");
        return NULL;
    }
    node->record = *record;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/**
 * Вставляет запись в двоичное сортированное дерево.
 * Если запись с такими же ключами (автор + название) уже есть,
 * она не добавляется (избегаем дубликатов).
 *
 * @param root Указатель на корень дерева (может быть NULL).
 * @param record Указатель на вставляемую запись.
 * @return Указатель на корень дерева.
 */
static TreeNode *tree_insert(TreeNode *root, const BibRecord *record) {
    if (root == NULL)
        return tree_node_create(record);

    int cmp = compare_records(record, &root->record);

    if (cmp < 0)
        root->left = tree_insert(root->left, record);
    else if (cmp > 0)
        root->right = tree_insert(root->right, record);
    /* else: cmp == 0 — дубликат, пропускаем */

    return root;
}

/**
 * Рекурсивно обходит дерево в порядке in-order (левое поддерево, узел, правое поддерево)
 * и записывает записи в файл.
 *
 * @param root Указатель на текущий узел.
 * @param f Указатель на выходной файл.
 */
static void tree_inorder_write(TreeNode *root, FILE *f) {
    if (root == NULL)
        return;

    tree_inorder_write(root->left, f);

    fprintf(f, "@book{book,\n");
    fprintf(f, "   title =     {%s},\n", root->record.title);
    fprintf(f, "   author =    {%s},\n", root->record.author);
    fprintf(f, "   publisher = {%s},\n", root->record.publisher);
    fprintf(f, "   isbn =      {%s},\n", root->record.isbn);
    fprintf(f, "   year =      {%s},\n", root->record.year);
    fprintf(f, "   series =    {%s},\n", root->record.series);
    fprintf(f, "   edition =   {%s},\n", root->record.edition);
    fprintf(f, "   volume =    {%s},\n", root->record.volume);
    fprintf(f, "   url =       {%s},\n", root->record.url);
    fprintf(f, "}\n\n");

    tree_inorder_write(root->right, f);
}

/**
 * Рекурсивно освобождает память, занятую деревом.
 * @param root Указатель на корень дерева.
 */
static void tree_destroy(TreeNode *root) {
    if (root == NULL)
        return;
    tree_destroy(root->left);
    tree_destroy(root->right);
    free(root);
}

/* ---------------------------------------------------------------
 * Парсинг bib-файлов
 * ---------------------------------------------------------------
 */

/**
 * Извлекает содержимое поля вида "key = {value}" из строки.
 * Функция ищет фигурные скобки и извлекает текст между ними.
 * Учитывает вложенность скобок.
 *
 * @param line Строка вида '   title =     {Principles of data mining},'
 * @param value Буфер для извлечённого значения.
 * @param value_size Размер буфера.
 * @return 1 если значение найдено, 0 если нет.
 */
static int extract_bib_field(const char *line, char *value, size_t value_size) {
    const char *brace_open = NULL;
    const char *brace_close = NULL;
    size_t len;

    /* Пропускаем пробельные символы в начале */
    while (*line && (unsigned char)*line <= ' ')
        line++;

    if (*line == '\0')
        return 0;

    /* Ищем '=' */
    const char *eq = strchr(line, '=');
    if (!eq)
        return 0;

    /* Ищем открывающую скобку после '=' */
    brace_open = strchr(eq, '{');
    if (!brace_open)
        return 0;

    /* Ищем закрывающую скобку — с учётом вложенности */
    brace_open++; /* Пропускаем '{' */
    brace_close = brace_open;
    int depth = 1;

    while (*brace_close && depth > 0) {
        if (*brace_close == '{') {
            depth++;
        } else if (*brace_close == '}') {
            depth--;
        }
        if (depth > 0)
            brace_close++;
    }

    if (depth != 0)
        return 0; /* Непарные скобки */

    len = (size_t)(brace_close - brace_open);
    if (len >= value_size)
        len = value_size - 1;

    strncpy(value, brace_open, len);
    value[len] = '\0';

    return 1;
}

/**
 * Обрабатывает одну bib-запись: заполняет структуру BibRecord
 * из массива строк. Поля могут быть в любом порядке.
 *
 * @param lines Массив строк записи.
 * @param line_count Количество строк.
 * @param record Указатель на заполняемую структуру.
 */
static void parse_bib_record(const char **lines, int line_count, BibRecord *record) {
    /* Очищаем структуру */
    memset(record, 0, sizeof(BibRecord));

    for (int i = 0; i < line_count; i++) {
        const char *line = lines[i];

        /* Определяем имя поля по первым символам до '=' */
        while (*line && (unsigned char)*line <= ' ')
            line++;

        if (strncmp(line, "title", 5) == 0) {
            extract_bib_field(line, record->title, sizeof(record->title));
        } else if (strncmp(line, "author", 6) == 0) {
            extract_bib_field(line, record->author, sizeof(record->author));
        } else if (strncmp(line, "publisher", 9) == 0) {
            extract_bib_field(line, record->publisher, sizeof(record->publisher));
        } else if (strncmp(line, "isbn", 4) == 0) {
            extract_bib_field(line, record->isbn, sizeof(record->isbn));
        } else if (strncmp(line, "year", 4) == 0) {
            extract_bib_field(line, record->year, sizeof(record->year));
        } else if (strncmp(line, "series", 6) == 0) {
            extract_bib_field(line, record->series, sizeof(record->series));
        } else if (strncmp(line, "edition", 7) == 0) {
            extract_bib_field(line, record->edition, sizeof(record->edition));
        } else if (strncmp(line, "volume", 6) == 0) {
            extract_bib_field(line, record->volume, sizeof(record->volume));
        } else if (strncmp(line, "url", 3) == 0) {
            extract_bib_field(line, record->url, sizeof(record->url));
        }
    }
}

/**
 * Читает bib-файл и извлекает из него записи, вставляя их в дерево.
 *
 * Алгоритм парсинга:
 * 1. Читаем файл построчно.
 * 2. Строка, начинающаяся с '@', сигнализирует начало новой записи.
 * 3. Строки между '@' и пустой строкой собираются в запись.
 * 4. Каждая запись парсится, конвертируется в cp1251 и вставляется в дерево.
 *
 * @param filename Имя файла для обработки.
 * @param root Указатель на корень дерева (может быть изменён).
 * @return Указатель на корень дерева.
 */
static TreeNode *process_bib_file(const char *filename, TreeNode *root) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Предупреждение: не удалось открыть файл '%s'.\n", filename);
        return root;
    }

    /* Временный буфер для строк в UTF-8 */
    char line_utf8[MAX_LINE_LEN];

    /* Массив для хранения строк текущей записи (до пустой строки) */
    const char *record_lines[MAX_BIB_FIELDS];
    int record_line_count = 0;
    int in_record = 0;

    while (fgets(line_utf8, sizeof(line_utf8), f) != NULL) {
        /* Удаляем завершающий перевод строки */
        size_t len = strlen(line_utf8);
        while (len > 0 && (line_utf8[len - 1] == '\n' || line_utf8[len - 1] == '\r'))
            line_utf8[--len] = '\0';

        /* Пропускаем пустые строки */
        const char *trimmed = line_utf8;
        while (*trimmed && (unsigned char)*trimmed <= ' ')
            trimmed++;

        if (*trimmed == '\0') {
            /* Пустая строка — конец записи (если мы внутри неё) */
            if (in_record && record_line_count > 0) {
                /* Парсим запись */
                BibRecord record;
                parse_bib_record(record_lines, record_line_count, &record);

                /* Конвертируем поля из UTF-8 в cp1251 */
                char converted[MAX_AUTHOR_LEN];
                utf8_to_cp1251(record.author, converted, sizeof(converted));
                strncpy(record.author, converted, sizeof(record.author) - 1);

                utf8_to_cp1251(record.title, converted, sizeof(converted));
                strncpy(record.title, converted, sizeof(record.title) - 1);

                utf8_to_cp1251(record.publisher, converted, sizeof(converted));
                strncpy(record.publisher, converted, sizeof(record.publisher) - 1);

                utf8_to_cp1251(record.isbn, converted, sizeof(converted));
                strncpy(record.isbn, converted, sizeof(record.isbn) - 1);

                utf8_to_cp1251(record.year, converted, sizeof(converted));
                strncpy(record.year, converted, sizeof(record.year) - 1);

                utf8_to_cp1251(record.series, converted, sizeof(converted));
                strncpy(record.series, converted, sizeof(record.series) - 1);

                utf8_to_cp1251(record.edition, converted, sizeof(converted));
                strncpy(record.edition, converted, sizeof(record.edition) - 1);

                utf8_to_cp1251(record.volume, converted, sizeof(converted));
                strncpy(record.volume, converted, sizeof(record.volume) - 1);

                utf8_to_cp1251(record.url, converted, sizeof(converted));
                strncpy(record.url, converted, sizeof(record.url) - 1);

                /* Вставляем в дерево */
                root = tree_insert(root, &record);

                /* Сбрасываем состояние */
                record_line_count = 0;
                in_record = 0;
            }
            continue;
        }

        /* Строка, начинающаяся с '@' — начало новой записи */
        if (*trimmed == '@') {
            /* Если была незавершённая запись, сбрасываем */
            if (in_record && record_line_count > 0) {
                BibRecord record;
                parse_bib_record(record_lines, record_line_count, &record);
                /* Аналогичная конвертация */
                char converted[MAX_AUTHOR_LEN];

                utf8_to_cp1251(record.author, converted, sizeof(converted));
                strncpy(record.author, converted, sizeof(record.author) - 1);

                utf8_to_cp1251(record.title, converted, sizeof(converted));
                strncpy(record.title, converted, sizeof(record.title) - 1);

                utf8_to_cp1251(record.publisher, converted, sizeof(converted));
                strncpy(record.publisher, converted, sizeof(record.publisher) - 1);

                utf8_to_cp1251(record.isbn, converted, sizeof(converted));
                strncpy(record.isbn, converted, sizeof(record.isbn) - 1);

                utf8_to_cp1251(record.year, converted, sizeof(converted));
                strncpy(record.year, converted, sizeof(record.year) - 1);

                utf8_to_cp1251(record.series, converted, sizeof(converted));
                strncpy(record.series, converted, sizeof(record.series) - 1);

                utf8_to_cp1251(record.edition, converted, sizeof(converted));
                strncpy(record.edition, converted, sizeof(record.edition) - 1);

                utf8_to_cp1251(record.volume, converted, sizeof(converted));
                strncpy(record.volume, converted, sizeof(record.volume) - 1);

                utf8_to_cp1251(record.url, converted, sizeof(converted));
                strncpy(record.url, converted, sizeof(record.url) - 1);

                root = tree_insert(root, &record);
                record_line_count = 0;
            }
            in_record = 1;
            /* Саму строку @book{...} не сохраняем */
            continue;
        }

        /* Если мы внутри записи и строка содержит '=', сохраняем её */
        if (in_record && strchr(trimmed, '=') != NULL) {
            if (record_line_count < MAX_BIB_FIELDS) {
                /* Выделяем память и копируем строку */
                char *line_copy = strdup(line_utf8);
                if (line_copy)
                    record_lines[record_line_count++] = line_copy;
            }
        }
    }

    /* Обрабатываем последнюю запись, если файл не заканчивается пустой строкой */
    if (in_record && record_line_count > 0) {
        BibRecord record;
        parse_bib_record(record_lines, record_line_count, &record);

        char converted[MAX_AUTHOR_LEN];
        utf8_to_cp1251(record.author, converted, sizeof(converted));
        strncpy(record.author, converted, sizeof(record.author) - 1);

        utf8_to_cp1251(record.title, converted, sizeof(converted));
        strncpy(record.title, converted, sizeof(record.title) - 1);

        utf8_to_cp1251(record.publisher, converted, sizeof(converted));
        strncpy(record.publisher, converted, sizeof(record.publisher) - 1);

        utf8_to_cp1251(record.isbn, converted, sizeof(converted));
        strncpy(record.isbn, converted, sizeof(record.isbn) - 1);

        utf8_to_cp1251(record.year, converted, sizeof(converted));
        strncpy(record.year, converted, sizeof(record.year) - 1);

        utf8_to_cp1251(record.series, converted, sizeof(converted));
        strncpy(record.series, converted, sizeof(record.series) - 1);

        utf8_to_cp1251(record.edition, converted, sizeof(converted));
        strncpy(record.edition, converted, sizeof(record.edition) - 1);

        utf8_to_cp1251(record.volume, converted, sizeof(converted));
        strncpy(record.volume, converted, sizeof(record.volume) - 1);

        utf8_to_cp1251(record.url, converted, sizeof(converted));
        strncpy(record.url, converted, sizeof(record.url) - 1);

        root = tree_insert(root, &record);
    }

    /* Освобождаем сохранённые строки */
    for (int i = 0; i < record_line_count; i++) {
        free((void *)record_lines[i]);
    }

    fclose(f);
    return root;
}

/* ---------------------------------------------------------------
 * Точка входа
 * ---------------------------------------------------------------
 */

int main(int argc, char *argv[]) {
    TreeNode *root = NULL;

    if (argc < 2) {
        fprintf(stderr, "Использование: %s <bibfile1.bib> [bibfile2.bib ...]\n", argv[0]);
        fprintf(stderr, "Программа читает .bib файлы, строит двоичное сортированное дерево\n");
        fprintf(stderr, "и выводит упорядоченные записи в файл 'output.txt' (кодировка cp1251).\n");
        return 1;
    }

    /* Обрабатываем каждый файл из аргументов командной строки */
    for (int i = 1; i < argc; i++) {
        root = process_bib_file(argv[i], root);
    }

    /* Открываем выходной файл в бинарном режиме для записи cp1251 */
    FILE *fout = fopen("output/output.txt", "wb");
    if (!fout) {
        fprintf(stderr, "Ошибка: не удалось создать выходной файл 'output.txt'.\n");
        tree_destroy(root);
        return 1;
    }

    /* Записываем заголовок латиницей (чтобы не зависеть от кодировки исходника) */
    const char *header = "; Bibliographic sources sorted by author/title (cp1251)\r\n";
    fwrite(header, 1, strlen(header), fout);

    /* Обходим дерево в порядке in-order и записываем записи */
    tree_inorder_write(root, fout);

    fclose(fout);
    printf("Готово. Упорядоченные записи сохранены в файл 'output.txt' (кодировка cp1251).\n");

    /* Освобождаем память */
    tree_destroy(root);

    return 0;
}
