/*
 * Лабораторная работа №1
 * Тема: Решето Эратосфена (битовая версия)
 *
 * Описание:
 * Программа находит все простые числа не больше заданного n
 * методом решета Эратосфена с использованием битового массива.
 * Каждый бит соответствует числу: 1 — простое, 0 — составное.
 * Оптимизация: вычёркивание начинается с p^2.
 *
 * Ввод: целое число n (n >= 2).
 * Вывод: все простые числа от 2 до n.
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Размер битового массива (в байтах) для хранения n + 1 бит.
 * (n + 1) — чтобы индекс совпадал с самим числом.
 * Делим на 8, добавляем 1 для остатка.
 */
#define BITMAP_SIZE(n)  (((n) + 1 + 7) / 8)

/**
 * Устанавливает бит с индексом idx в 1.
 *
 * @param bitmap Указатель на битовый массив.
 * @param idx    Индекс бита.
 */
static inline void	bit_set(unsigned char *bitmap, int idx)
{
	bitmap[idx >> 3] |= (unsigned char)(1 << (idx & 7));
}

/**
 * Устанавливает бит с индексом idx в 0.
 *
 * @param bitmap Указатель на битовый массив.
 * @param idx    Индекс бита.
 */
static inline void	bit_clear(unsigned char *bitmap, int idx)
{
	bitmap[idx >> 3] &= (unsigned char)~(1 << (idx & 7));
}

/**
 * Возвращает значение бита с индексом idx (0 или 1).
 *
 * @param  bitmap Указатель на битовый массив.
 * @param  idx    Индекс бита.
 * @return        1, если бит установлен; 0 — иначе.
 */
static inline int	bit_get(const unsigned char *bitmap, int idx)
{
	return (bitmap[idx >> 3] >> (idx & 7)) & 1;
}

/**
 * Находит все простые числа не больше n методом решета Эратосфена
 * с использованием битового массива.
 *
 * Хранение:
 *   - бит с индексом i = 1, если i — простое число;
 *   - бит с индексом i = 0, если i — составное или не используется (0, 1).
 *
 * Оптимизация: вычёркивание начинается с p^2,
 *              внешний цикл останавливается при p * p > n.
 *
 * @param  n    Верхняя граница поиска (n >= 2).
 * @param  size Выходной параметр: количество найденных простых чисел.
 * @return      Указатель на динамически выделенный массив простых чисел,
 *              или NULL при ошибке выделения памяти.
 *              Вызывающий отвечает за освобождение памяти через free().
 */
int	*sieve_of_eratosthenes(int n, int *size)
{
	int		bytes;
	unsigned char	*bitmap;
	int		p;
	int		i;
	int		*primes;
	int		count;

	bytes = BITMAP_SIZE(n);
	bitmap = (unsigned char *)malloc((size_t)bytes * sizeof(unsigned char));
	if (bitmap == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для битового массива.\n");
		*size = 0;
		return NULL;
	}

	/* Инициализация: все биты = 1 (все числа считаем простыми) */
	memset(bitmap, 0xFF, (size_t)bytes);

	/* 0 и 1 — не простые */
	bit_clear(bitmap, 0);
	bit_clear(bitmap, 1);

	/* Шаг 2: первое простое число */
	p = 2;

	/* Шаги 3-5: основной цикл решета, до p*p > n */
	while (p * p <= n)
	{
		/*
		 * Шаг 3 (оптимизированный): вычёркиваем числа, кратные p,
		 * начиная с p*p.
		 */
		for (i = p * p; i <= n; i += p)
		{
			bit_clear(bitmap, i);
		}

		/* Шаг 4: ищем следующее невычеркнутое число (бит = 1) */
		do {
			++p;
		} while (p <= n && bit_get(bitmap, p) == 0);
	}

	/* Шаг 6: собираем простые числа в массив */
	count = 0;
	for (i = 2; i <= n; ++i)
	{
		if (bit_get(bitmap, i))
		{
			++count;
		}
	}

	primes = (int *)malloc((size_t)count * sizeof(int));
	if (primes == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память для результата.\n");
		free(bitmap);
		*size = 0;
		return NULL;
	}

	count = 0;
	for (i = 2; i <= n; ++i)
	{
		if (bit_get(bitmap, i))
		{
			primes[count++] = i;
		}
	}

	free(bitmap);
	*size = count;
	return primes;
}

/**
 * Выводит массив целых чисел через пробел и перевод строки в конце.
 *
 * @param arr  Указатель на массив целых чисел.
 * @param size Количество элементов в массиве.
 */
static void	print_array(const int arr[], int size)
{
	for (int i = 0; i < size; ++i)
	{
		printf("%d ", arr[i]);
	}
	printf("\n");
}

/**
 * Точка входа в программу.
 *
 * Запрашивает у пользователя число n,
 * вычисляет все простые числа до n и выводит их на экран.
 */
int	main(void)
{
	int	n;
	int	size;
	int	*primes;

	printf("Введите число n (n >= 2): ");
	if (scanf("%d", &n) != 1 || n < 2)
	{
		fprintf(stderr, "Ошибка: n должно быть целым числом >= 2.\n");
		return 1;
	}

	primes = sieve_of_eratosthenes(n, &size);
	if (primes == NULL)
	{
		return 1;
	}

	printf("Простые числа от 2 до %d:\n", n);
	print_array(primes, size);

	free(primes);
	return 0;
}
