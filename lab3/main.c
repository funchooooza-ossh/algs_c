/*
 * Лабораторная работа №3
 * Тема: Двоичные сортированные деревья
 *
 * Описание:
 * Программа считывает несколько (до 256) текстовых файлов из каталога "files",
 * разбивает текст на слова, подсчитывает частоту каждого слова
 * с использованием бинарного дерева поиска (BST)
 * и выводит частотный словарь, упорядоченный в алфавитном порядке, в файл.
 *
 * Слово: последовательность букв и цифр, начинающаяся с буквы.
 * Все слова приводятся к нижнему регистру.
 * Морфология (словоформы) не учитывается.
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main files/f1.txt [files/f2.txt ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_FILES 256
/**
 * Структура узла бинарного дерева.
 * left, right — ссылки на левого и правого потомка.
 * w — строка слова (динамически выделенная).
 * count — количество вхождений слова.
 */
typedef struct node
{
	int count;
	char *w;
	struct node *left;
	struct node *right;
} NODE;

/** Корень дерева (глобальная переменная). */
NODE *tree = NULL;
/**
 * Создаёт новый узел дерева.
 *
 * Выделяет память под структуру NODE и копирует строку word.
 *
 * @param  word Строка слова.
 * @return      Указатель на новый узел.
 */
static NODE	*node_new(const char *word)
{
	NODE *new_node;

	new_node = (NODE *)malloc(sizeof(NODE));
	if (new_node == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для узла.\n");
		exit(1);
	}
	new_node->w = (char *)malloc((strlen(word) + 1) * sizeof(char));
	if (new_node->w == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для слова.\n");
		free(new_node);
		exit(1);
	}
	strcpy(new_node->w, word);
	new_node->count = 1;
	new_node->left = NULL;
	new_node->right = NULL;

	return new_node;
}

/**
 * Рекурсивно добавляет слово в дерево (или увеличивает счётчик).
 *
 * Если слово уже существует — увеличивает count.
 * Если отсутствует — создаёт новый узел и вставляет в нужную позицию
 * (по алфавитному порядку, через strcmp).
 *
 * @param root  Указатель на указатель корня (поддерева).
 * @param word  Строка слова (в нижнем регистре).
 */
static void	tree_insert(NODE **root, const char *word)
{
	int cmp;
	if (*root == NULL)
	{
		*root = node_new(word);
		return;
	}

	cmp = strcmp(word, (*root)->w);
	if (cmp == 0)
	{
		/* Слово уже есть — увеличиваем счётчик */
		++(*root)->count;
	}
	else if (cmp < 0)
	{
		tree_insert(&(*root)->left, word);
	}
	else
	{
		tree_insert(&(*root)->right, word);
	}
}

/**
 * Ищет слово в дереве по первым 5 символам (префикс).
 *
 * Использует strncmp(word, node->w, 5) для сравнения.
 * Если найдено хотя бы одно слово с таким префиксом —
 * возвращает указатель на первый найденный узел.
 * Если нет — возвращает NULL.
 *
 * @param  root  Корень поддерева.
 * @param  word  Строка-префикс (первые 5 символов).
 * @return       Указатель на найденный узел или NULL.
 */
NODE	*tree_search_prefix(NODE *root, const char *word)
{
	int cmp;

	if (root == NULL)
	{
		return NULL;
	}

	cmp = strncmp(word, root->w, 5);
	if (cmp == 0)
	{
		return root;
	}
	if (cmp < 0)
	{
		return tree_search_prefix(root->left, word);
	}
	return tree_search_prefix(root->right, word);
}

/**
 * Рекурсивно обходит дерево в симметричном порядке (in-order)
 * и записывает слова в файл в формате "слово количество".
 *
 * In-order обход даёт алфавитную сортировку.
 *
 * @param root Указатель на корень поддерева.
 * @param file Указатель на открытый файл для записи.
 */
static void	tree_print_inorder(const NODE *root, FILE *file)
{
	if (root == NULL)
	{
		return;
	}

	tree_print_inorder(root->left, file);
	fprintf(file, "%s %d\n", root->w, root->count);
	tree_print_inorder(root->right, file);
}

/**
 * Рекурсивно освобождает память, занятую узлами дерева.
 *
 * @param root Указатель на корень поддерева.
 */
static void	tree_free(NODE *root)
{
	if (root == NULL)
	{
		return;
	}

	tree_free(root->left);
	tree_free(root->right);
	free(root->w);
	free(root);
}

/**
 * Приводит строку к нижнему регистру (in-place).
 *
 * @param str Строка для преобразования.
 */
static void	to_lowercase(char *str)
{
	for (int i = 0; str[i] != '\0'; ++i)
	{
		str[i] = (char)tolower((unsigned char)str[i]);
	}
}

/**
 * Обрабатывает один файл: читает, разбивает на слова,
 * добавляет слова в дерево.
 *
 * Разделители: все символы, не являющиеся буквой или цифрой.
 * Слово должно начинаться с буквы.
 *
 * @param filename Имя файла для обработки.
 */
static void	process_file(const char *filename)
{
	FILE	*file;
	long	file_size;
	char	*buffer;
	char	delimiters[256];
	int	delim_count;
	char	*token;

	file = fopen(filename, "r");
	if (file == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось открыть файл \"%s\".\n", filename);
		return;
	}

	/* Определяем размер файла */
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);

	/* Выделяем буфер для всего файла */
	buffer = (char *)malloc((size_t)(file_size + 1) * sizeof(char));
	if (buffer == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для буфера.\n");
		fclose(file);
		exit(1);
	}

	fread(buffer, sizeof(char), (size_t)file_size, file);
	buffer[file_size] = '\0';

	fclose(file);

	/* Строим строку разделителей: все символы, не буквы и не цифры */
	delim_count = 0;
	for (int i = 0; i < 256; ++i)
	{
		if (!isalnum(i) && i != '\0')
		{
			delimiters[delim_count++] = (char)i;
		}
	}
	delimiters[delim_count] = '\0';

	/* Разбиваем на слова через strtok */
	token = strtok(buffer, delimiters);
	while (token != NULL)
	{
		if (isalpha((unsigned char)token[0]))
		{
			to_lowercase(token);
			tree_insert(&tree, token);
		}
		token = strtok(NULL, delimiters);
	}

	free(buffer);
}

/**
 * Точка входа в программу.
 *
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов.
 * @return 0 при успехе, 1 при ошибке.
 */
int	main(int argc, char *argv[])
{
	FILE	*output;
	int	file_count;

	if (argc < 2)
	{
		fprintf(stderr, "Использование: %s файл1 [файл2 ... файлN]\n", argv[0]);
		fprintf(stderr, "где N <= %d\n", MAX_FILES);
		return 1;
	}

	file_count = argc - 1;
	if (file_count > MAX_FILES)
	{
		fprintf(stderr, "Ошибка: количество файлов (%d) превышает лимит (%d).\n",
				file_count, MAX_FILES);
		return 1;
	}

	/* Обрабатываем каждый файл */
	for (int i = 0; i < file_count; ++i)
	{
		printf("Обработка файла: %s\n", argv[i + 1]);
		process_file(argv[i + 1]);
	}
	printf("Найдено уникальных слов.\n");

	/* Сохраняем результат в файл */
	output = fopen("output/dict_alpha.txt", "w");
	if (output == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось создать файл dict_alpha.txt.\n");
		tree_free(tree);
		return 1;
	}

	tree_print_inorder(tree, output);
	fclose(output);

	printf("Создан файл: dict_alpha.txt (по алфавиту)\n");

	/* Освобождаем память */
	tree_free(tree);
	return 0;
}

