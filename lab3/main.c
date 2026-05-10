/*
 * Лабораторная работа №3
 * Тема: Частотный словарь слов
 *
 * Описание:
 * Программа считывает несколько (до 256) текстовых файлов,
 * разбивает текст на слова, подсчитывает частоту каждого слова
 * и выводит два отсортированных словаря:
 *   1) dict_alpha.txt — по алфавиту
 *   2) dict_freq.txt  — по убыванию частоты
 *
 * Слово: последовательность букв и цифр, начинающаяся с буквы.
 * Все слова приводятся к нижнему регистру.
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main texts/1ws4610.txt [texts/2.txt ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORDS 100000
#define MAX_FILES 256

/**
 * Структура для хранения слова и его частоты.
 */
typedef struct word
{
	int count;    // количество вхождений
	char *w;      // указатель на строку слова
} WORD;

/**
 * Массив указателей на структуры WORD.
 * Статический, фиксированного размера.
 */
WORD *words[MAX_WORDS];

int word_count = 0;  // текущее количество уникальных слов

/**
 * Приводит строку к нижнему регистру (in-place).
 *
 * @param str Строка для преобразования.
 */
void to_lowercase(char *str)
{
	for (int i = 0; str[i] != '\0'; ++i)
	{
		str[i] = (char)tolower((unsigned char)str[i]);
	}
}

/**
 * Ищет слово в массиве words.
 *
 * @param word Строка для поиска.
 * @return Индекс найденного слова, или -1 если не найдено.
 */
int find_word(const char *word)
{
	for (int i = 0; i < word_count; ++i)
	{
		if (strcmp(words[i]->w, word) == 0)
		{
			return i;
		}
	}
	return -1;
}

/**
 * Добавляет новое слово в массив words.
 * Выделяет память через malloc для структуры WORD и для строки.
 *
 * @param word Строка с новым словом.
 */
void add_word(const char *word)
{
	if (word_count >= MAX_WORDS)
	{
		fprintf(stderr, "Ошибка: превышен лимит уникальных слов (%d).\n", MAX_WORDS);
		return;
	}

	// Выделяем память под структуру WORD
	WORD *new_word = (WORD*)malloc(sizeof(WORD));
	if (new_word == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для структуры WORD.\n");
		exit(1);
	}

	// Выделяем память под строку слова (+1 для терминирующего нуля)
	new_word->w = (char*)malloc((strlen(word) + 1) * sizeof(char));
	if (new_word->w == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для строки слова.\n");
		free(new_word);
		exit(1);
	}

	strcpy(new_word->w, word);
	new_word->count = 1;

	words[word_count] = new_word;
	++word_count;
}

/**
 * Обрабатывает один файл: читает, разбивает на слова, обновляет словарь.
 *
 * Разбивка на слова выполняется с помощью strtok.
 * Разделители: все символы, не являющиеся буквой или цифрой.
 * Слово должно начинаться с буквы.
 *
 * @param filename Имя файла для обработки.
 */
void process_file(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось открыть файл \"%s\".\n", filename);
		return;
	}

	// Читаем весь файл в память (однопроходный алгоритм)
	// Сначала определяем размер
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	rewind(file);

	// Выделяем буфер (+1 для '\0')
	char *buffer = (char*)malloc((size_t)(file_size + 1) * sizeof(char));
	if (buffer == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для буфера файла.\n");
		fclose(file);
		exit(1);
	}

	size_t bytes_read = fread(buffer, sizeof(char), (size_t)file_size, file);
	buffer[bytes_read] = '\0';

	fclose(file);

	// Строим строку разделителей: все символы, не являющиеся буквой или цифрой
	// Для strtok нужна строка с разделителями. Используем таблицу ASCII.
	char delimiters[256];
	int delim_count = 0;

	for (int i = 0; i < 256; ++i)
	{
		if (!isalnum(i) && i != '\0')
		{
			delimiters[delim_count++] = (char)i;
		}
	}
	delimiters[delim_count] = '\0';

	// Разбиваем на слова через strtok
	char *token = strtok(buffer, delimiters);
	while (token != NULL)
	{
		// Проверяем, что слово начинается с буквы
		if (isalpha((unsigned char)token[0]))
		{
			// Приводим к нижнему регистру
			to_lowercase(token);

			// Ищем слово в словаре
			int idx = find_word(token);
			if (idx != -1)
			{
				++words[idx]->count;
			}
			else
			{
				add_word(token);
			}
		}

		token = strtok(NULL, delimiters);
	}

	free(buffer);
}

/**
 * Функция сравнения для qsort — сортировка по алфавиту (по возрастанию).
 *
 * @param a Указатель на элемент массива (WORD**).
 * @param b Указатель на элемент массива (WORD**).
 * @return Результат strcmp.
 */
int compare_alpha(const void *a, const void *b)
{
	const WORD *wa = *(const WORD**)a;
	const WORD *wb = *(const WORD**)b;
	return strcmp(wa->w, wb->w);
}

/**
 * Функция сравнения для qsort — сортировка по убыванию частоты.
 * При равной частоте — по алфавиту.
 *
 * @param a Указатель на элемент массива (WORD**).
 * @param b Указатель на элемент массива (WORD**).
 * @return Отрицательное, ноль или положительное число.
 */
int compare_freq(const void *a, const void *b)
{
	const WORD *wa = *(const WORD**)a;
	const WORD *wb = *(const WORD**)b;

	if (wb->count != wa->count)
	{
		return wb->count - wa->count;  // по убыванию
	}

	return strcmp(wa->w, wb->w);  // при равенстве — по алфавиту
}
/**
 * Сохраняет словарь в файл.
 *
 * @param filename Имя выходного файла.
 * @param mode Режим сортировки: "alpha" или "freq".
 */
void save_dict(const char *filename, const char *mode)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось создать файл \"%s\".\n", filename);
		return;
	}

	// Сортируем копию массива указателей, чтобы не портить оригинал
	WORD **sorted = (WORD**)malloc((size_t)word_count * sizeof(WORD*));
	if (sorted == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для сортировки.\n");
		fclose(file);
		exit(1);
	}

	for (int i = 0; i < word_count; ++i)
	{
		sorted[i] = words[i];
	}

	if (strcmp(mode, "alpha") == 0)
	{
		qsort(sorted, (size_t)word_count, sizeof(WORD*), compare_alpha);
	}
	else if (strcmp(mode, "freq") == 0)
	{
		qsort(sorted, (size_t)word_count, sizeof(WORD*), compare_freq);
	}

	// Записываем в файл
	for (int i = 0; i < word_count; ++i)
	{
		fprintf(file, "%s %d\n", sorted[i]->w, sorted[i]->count);
	}

	fclose(file);
	free(sorted);
}

/**
 * Освобождает всю выделенную динамическую память.
 */
void cleanup(void)
{
	for (int i = 0; i < word_count; ++i)
	{
		free(words[i]->w);
		free(words[i]);
	}
}

/**
 * Точка входа в программу.
 *
 * @param argc Количество аргументов командной строки.
 * @param argv Массив указателей на аргументы командной строки.
 * @return 0 при успехе, 1 при ошибке.
 */
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Использование: %s файл1 [файл2 ... файлN]\n", argv[0]);
		fprintf(stderr, "где N <= %d\n", MAX_FILES);
		return 1;
	}

	int file_count = argc - 1;
	if (file_count > MAX_FILES)
	{
		fprintf(stderr, "Ошибка: количество файлов (%d) превышает лимит (%d).\n",
				file_count, MAX_FILES);
		return 1;
	}

	// Обрабатываем каждый файл
	for (int i = 0; i < file_count; ++i)
	{
		printf("Обработка файла: %s\n", argv[i + 1]);
		process_file(argv[i + 1]);
	}

	printf("Найдено уникальных слов: %d\n", word_count);

	// Сохраняем словарь по алфавиту
	save_dict("output/dict_alpha.txt", "alpha");
	printf("Создан файл: dict_alpha.txt (по алфавиту)\n");

	// Сохраняем словарь по частоте
	save_dict("output/dict_freq.txt", "freq");
	printf("Создан файл: dict_freq.txt (по частоте)\n");

	// Освобождаем память
	cleanup();

	return 0;
}

