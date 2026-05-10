/*
 * Лабораторная работа №1
 * Тема: Решето Эратосфена
 *
 * Описание:
 * Программа находит все простые числа не больше заданного n
 * методом решета Эратосфена с оптимизацией (начало вычеркивания с p^2).
 *
 * Ввод: целое число n (n >= 2).
 * Вывод: все простые числа от 2 до n.
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main -lm
 * Запуск: ./output/main
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * Выводит массив целых чисел через пробел и перевод строки в конце.
 *
 * @param arr  Указатель на массив целых чисел.
 * @param size Количество элементов в массиве.
 */
void print_array(const int arr[], const int size)
{
	for (int i = 0; i < size; ++i)
	{
		printf("%d ", arr[i]);
	}
	printf("\n");
}

/**
 * Удаляет из массива все нулевые элементы, сжимая массив.
 *
 * @param arr  Указатель на массив (будет изменён).
 * @param size Указатель на текущее количество элементов.
 *             После выполнения содержит количество ненулевых элементов.
 */
void remove_zeros(int arr[], int *size)
{
	int write_idx = 0;

	for (int read_idx = 0; read_idx < *size; ++read_idx)
	{
		if (arr[read_idx] != 0)
		{
			arr[write_idx++] = arr[read_idx];
		}
	}

	*size = write_idx;
}

/**
 * Находит все простые числа не больше n методом решета Эратосфена
 * с оптимизацией: вычёркивание начинается с p^2, а не с 2p.
 *
 * @param  n    Верхняя граница поиска (n >= 2).
 * @param  size Выходной параметр: количество найденных простых чисел.
 * @return      Указатель на динамически выделенный массив простых чисел,
 *              или NULL при ошибке выделения памяти.
 *              Вызывающий отвечает за освобождение памяти через free().
 */
int* sieve_of_eratosthenes(const int n, int *size)
{
	// Шаг 1: массив чисел от 2 до n включительно
	int *P = (int*)malloc((size_t)(n - 1) * sizeof(int));
	if (P == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось выделить память.\n");
		*size = 0;
		return NULL;
	}

	for (int i = 0; i < n - 1; ++i)
	{
		P[i] = i + 2;
	}

	int current_size = n - 1;
	int p = 2; // Шаг 2: первое простое число

	// Шаги 3-5: основной цикл решета
	while (p * p <= n)
	{
		// Шаг 3: вычёркиваем числа, кратные p, начиная с p*p
		for (int i = p * p; i <= n; i += p)
		{
			P[i - 2] = 0;
		}

		// Шаг 4: ищем следующее невычеркнутое число
		int next_p = 0;
		for (int i = p + 1; i <= n; ++i)
		{
			if (P[i - 2] != 0)
			{
				next_p = P[i - 2];
				break;
			}
		}

		if (next_p == 0)
		{
			break;
		}

		p = next_p;
	}

	// Шаги 6-7: удаляем вычеркнутые (нулевые) элементы
	remove_zeros(P, &current_size);

	*size = current_size;
	return P;
}

/**
 * Точка входа в программу.
 *
 * Запрашивает у пользователя число n,
 * вычисляет все простые числа до n и выводит их на экран.
 */
int main(void)
{
	int n;

	printf("Введите число n (n >= 2): ");
	if (scanf("%d", &n) != 1 || n < 2)
	{
		fprintf(stderr, "Ошибка: n должно быть целым числом >= 2.\n");
		return 1;
	}

	int size = 0;
	int *primes = sieve_of_eratosthenes(n, &size);

	if (primes == NULL)
	{
		return 1;
	}

	printf("Простые числа от 2 до %d:\n", n);
	print_array(primes, size);

	free(primes);

	return 0;
}
