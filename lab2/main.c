/*
 * Лабораторная работа №2
 * Тема: Посимвольный анализ текстового файла
 *
 * Описание:
 * Программа читает текстовый файл посимвольно (через fgetc),
 * подсчитывает различные категории символов и выводит статистику.
 *
 * Имя файла вводится с клавиатуры.
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * Проверяет, является ли символ гласной буквой английского алфавита.
 *
 * Гласные: a, e, i, o, u (как строчные, так и прописные).
 *
 * @param c Символ для проверки.
 * @return  1 если символ — гласная, 0 в противном случае.
 */
int is_vowel(const int c)
{
	const char vowels[] = "aeiouyAEIOUY";
	int i = 0;

	while (vowels[i] != '\0')
	{
		if (c == vowels[i])
		{
			return 1;
		}
		++i;
	}

	return 0;
}

/**
 * Проверяет, является ли символ согласной буквой английского алфавита.
 *
 * Согласные: буквы английского алфавита, не являющиеся гласными.
 *
 * @param c Символ для проверки.
 * @return  1 если символ — согласная, 0 в противном случае.
 */
int is_consonant(const int c)
{
	return isalpha(c) && !is_vowel(c);
}

/**
 * Структура для хранения статистики по символам в файле.
 */
typedef struct
{
	long total_chars;      // общее количество символов
	long lines;            // количество строк (\n)
	long digits;           // количество цифр
	long punct;            // количество знаков препинания
	long spaces;           // количество пробельных символов
	long letters;          // количество букв
	long lowercase;        // количество строчных букв
	long uppercase;        // количество прописных букв
	long vowels;           // количество гласных
	long consonants;       // количество согласных
} FileStats;

/**
 * Анализирует файл посимвольно и собирает статистику.
 *
 * Функция читает файл символ за символом с помощью fgetc,
 * классифицирует каждый символ с помощью макросов ctype.h
 * и собственных функций is_vowel / is_consonant.
 *
 * @param file Указатель на открытый файл.
 * @return     Структура FileStats с подсчитанными значениями.
 */
FileStats analyze_file(FILE *file)
{
	FileStats stats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int c;

	while ((c = fgetc(file)) != EOF)
	{
		++stats.total_chars;

		if (c == '\n')
		{
			++stats.lines;
		}

		if (isdigit(c))
		{
			++stats.digits;
		}

		if (ispunct(c))
		{
			++stats.punct;
		}

		if (isspace(c))
		{
			++stats.spaces;
		}

		if (isalpha(c))
		{
			++stats.letters;

			if (islower(c))
			{
				++stats.lowercase;
			}

			if (isupper(c))
			{
				++stats.uppercase;
			}

			if (is_vowel(c))
			{
				++stats.vowels;
			}

			if (is_consonant(c))
			{
				++stats.consonants;
			}
		}
	}

	return stats;
}

/**
 * Выводит статистику по файлу на стандартный вывод в читаемом виде.
 *
 * @param stats Структура со статистикой для вывода.
 */
void print_stats(const FileStats stats)
{
	printf("========== Статистика файла ==========\n");
	printf("Общее количество символов:    %ld\n", stats.total_chars);
	printf("Количество строк:             %ld\n", stats.lines);
	printf("Количество цифр:              %ld\n", stats.digits);
	printf("Количество знаков препинания: %ld\n", stats.punct);
	printf("Количество пробельных сим.:   %ld\n", stats.spaces);
	printf("Количество букв:              %ld\n", stats.letters);
	printf("  из них строчных:            %ld\n", stats.lowercase);
	printf("  из них прописных:           %ld\n", stats.uppercase);
	printf("  из них гласных:             %ld\n", stats.vowels);
	printf("  из них согласных:           %ld\n", stats.consonants);
	printf("======================================\n");
}

/**
 * Точка входа в программу.
 *
 * Запрашивает имя файла, открывает его,
 * выполняет посимвольный анализ и выводит результат.
 */
int main(void)
{
	char filename[256];
	FILE *file = NULL;

	printf("Введите имя файла: ");
	if (scanf("%255s", filename) != 1)
	{
		fprintf(stderr, "Ошибка: не удалось прочитать имя файла.\n");
		return 1;
	}

	file = fopen(filename, "r");
	if (file == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось открыть файл \"%s\".\n"
						"Проверьте, существует ли файл и есть ли права на чтение.\n",
				filename);
		return 1;
	}

	FileStats stats = analyze_file(file);

	fclose(file);

	print_stats(stats);

	return 0;
}
