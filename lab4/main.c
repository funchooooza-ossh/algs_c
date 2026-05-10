/*
 * Лабораторная работа №4
 * Тема: Глифы — анализ чёрно-белых изображений
 *
 * Описание:
 * Программа загружает глифы из каталога, переданного через аргумент,
 * для каждого глифа вычисляет характеристики:
 *   — количество чёрных пикселей (count)
 *   — плотность (density)
 *   — диаметр в манхэттенской метрике (diam)
 *   — периметр (perim)
 *   — связность по 4-связности (conn)
 * После анализа ищет похожие глифы среди глифов одинакового размера
 * (не более 10% различающихся пикселей).
 *
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main glyph/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#define ISBIT(n, x)		((((unsigned int)1 << (n)) & (x)) ? 1 : 0)
#define MAX_GLYPHS		50000
/*
 * ---------------------------------------------------------------
 * Структура глифа
 * ---------------------------------------------------------------
 */
typedef struct img
{
	int			w;			/* ширина в пикселях */
	int			h;			/* высота в пикселях */
	int			dx;			/* расстояние до следующего глифа (не используется) */
	int			count;		/* количество чёрных пикселей */
	int			id;			/* идентификатор — номер глифа */
	int			bytes;		/* количество байтов в битовой карте */
	double		density;	/* плотность чёрных пикселей */
	int			diam;		/* диаметр в манхэттенской метрике */
	int			perim;		/* периметр глифа */
	int			conn;		/* связность (количество компонентов 4-связности) */
	unsigned char *data;	/* битовая карта (неупакованная, 1 байт = 1 строка) */
} IMG;

IMG	*G[MAX_GLYPHS];
int	N;		/* количество загруженных глифов */

/*
 * ---------------------------------------------------------------
 * Вспомогательные функции (popcount)
 * ---------------------------------------------------------------
 */
static int	popcnt8(unsigned char i)
{
	int	count;

	count = 0;
	while (i)
	{
		++count;
		i = (i - 1) & i;
	}
	return (count);
}

static int	popcnt64(unsigned long long w)
{
	w -= (w >> 1) & 0x5555555555555555ULL;
	w = (w & 0x3333333333333333ULL) + ((w >> 2) & 0x3333333333333333ULL);
	w = (w + (w >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
	return ((int)((w * 0x0101010101010101ULL) >> 56));
}

/*
 * ---------------------------------------------------------------
 * Работа с пикселями глифа
 * ---------------------------------------------------------------
 * Битовая карта хранится построчно (1 строка = ceil(w/8) байт).
 * i — номер строки, j — номер столбца.
 */

/** Количество байтов в одной строке глифа. */
static int	row_bytes(const IMG *img)
{
	return ((img->w + 7) / 8);
}

/**
 * Проверяет, установлен ли пиксель (i, j) в 1 (чёрный).
 *
 * @param  img Указатель на глиф.
 * @param  i   Номер строки (0 .. h-1).
 * @param  j   Номер столбца (0 .. w-1).
 * @return     1 если пиксель чёрный, 0 если белый или за границами.
 */
static int	get_pixel(const IMG *img, int i, int j)
{
	int	rb;

	if (i < 0 || i >= img->h || j < 0 || j >= img->w)
		return (0);

	rb = row_bytes(img);
	return (ISBIT((7 - j % 8), img->data[i * rb + j / 8]));
}

/*
 * ---------------------------------------------------------------
 * Загрузка глифа из файла
 * ---------------------------------------------------------------
 */

/**
 * Загружает глиф из файла.
 *
 * Формат файла (бинарный):
 *   int w, int h, int dx, int count, int id, int bytes
 *   unsigned char data[bytes]
 *
 * @param  file Имя файла.
 * @return      Указатель на IMG или NULL при ошибке.
 */
static IMG	*load_img(const char *file)
{
	FILE	*F;
	IMG		*img;

	img = (IMG *)malloc(sizeof(IMG));
	if (img == NULL)
		return (NULL);

	F = fopen(file, "rb");
	if (F == NULL)
	{
		free(img);
		return (NULL);
	}

	fread(&(img->w), sizeof(int), 1, F);
	fread(&(img->h), sizeof(int), 1, F);
	fread(&(img->dx), sizeof(int), 1, F);
	fread(&(img->count), sizeof(int), 1, F);
	fread(&(img->id), sizeof(int), 1, F);
	fread(&(img->bytes), sizeof(int), 1, F);

	img->data = (unsigned char *)calloc(img->bytes, 1);
	if (img->data == NULL)
	{
		fclose(F);
		free(img);
		return (NULL);
	}

	fread(img->data, 1, img->bytes, F);
	fclose(F);

	img->density = 0.0;
	img->diam = 0;
	img->perim = 0;
	img->conn = 0;

	return (img);
}

/*
 * ---------------------------------------------------------------
 * Вычисление характеристик глифа
 * ---------------------------------------------------------------
 */

/**
 * Подсчитывает количество чёрных пикселей (count) и плотность.
 *
 * Использует popcnt64 для основных блоков по 8 байт и popcnt8
 * для остатка.
 */
static void	calc_count_and_density(IMG *img)
{
	unsigned long long	*t;
	unsigned char		*s;
	int					len;
	int					i;
	unsigned long long	c;

	t = (unsigned long long *)img->data;
	s = (unsigned char *)img->data;
	len = img->bytes;

	c = 0;
	for (i = 0; i < len / 8; ++i)
		c += popcnt64(t[i]);
	for (i = (len / 8) * 8; i < len; ++i)
		c += popcnt8(s[i]);

	img->count = (int)c;
	img->density = (double)c / (double)(img->w * img->h);
}

/**
 * Вычисляет диаметр глифа в манхэттенской метрике.
 *
 * Диаметр = максимальное манхэттенское расстояние
 * между любой парой чёрных пикселей:
 *   d((i1,j1), (i2,j2)) = |i1-i2| + |j1-j2|
 *
 * Оптимизация: если чёрных пикселей много (count > 5000),
 * используем аппроксимацию через bounding box,
 * иначе — полный перебор.
 */
static void	calc_diameter(IMG *img)
{
	int	i1, j1;
	int	max_dist;
	int	dist;
	int	count;

	max_dist = 0;
	count = 0;

	/* Собираем координаты чёрных пикселей */
	/* Используем статический буфер (до 10000 чёрных пикселей).
	 * Если больше — используем bounding box аппроксимацию.
	 */
	if (img->count > 10000)
	{
		/* Аппроксимация через bounding box */
		int	min_i = img->h, max_i = -1;
		int	min_j = img->w, max_j = -1;

		for (i1 = 0; i1 < img->h; ++i1)
		{
			for (j1 = 0; j1 < img->w; ++j1)
			{
				if (get_pixel(img, i1, j1))
				{
					if (i1 < min_i) min_i = i1;
					if (i1 > max_i) max_i = i1;
					if (j1 < min_j) min_j = j1;
					if (j1 > max_j) max_j = j1;
				}
			}
		}

		if (max_i >= min_i && max_j >= min_j)
			max_dist = (max_i - min_i) + (max_j - min_j);
	}
	else
	{
		/* Полный перебор всех пар чёрных пикселей */
		int	coords[10000][2];
		int	idx;

		idx = 0;
		for (i1 = 0; i1 < img->h && idx < 10000; ++i1)
		{
			for (j1 = 0; j1 < img->w && idx < 10000; ++j1)
			{
				if (get_pixel(img, i1, j1))
				{
					coords[idx][0] = i1;
					coords[idx][1] = j1;
					++idx;
				}
			}
		}

		count = idx;
		for (i1 = 0; i1 < count; ++i1)
		{
			for (j1 = i1 + 1; j1 < count; ++j1)
			{
				dist = abs(coords[i1][0] - coords[j1][0])
					+ abs(coords[i1][1] - coords[j1][1]);
				if (dist > max_dist)
					max_dist = dist;
			}
		}
	}

	img->diam = max_dist;
}

/**
 * Вычисляет периметр глифа.
 *
 * Периметр = количество чёрных пикселей, у которых
 * хотя бы один из 4-соседей (вверх, вниз, влево, вправо)
 * является белым (или выходит за границу изображения).
 */
static void	calc_perimeter(IMG *img)
{
	int	i, j;
	int	perim;

	perim = 0;
	for (i = 0; i < img->h; ++i)
	{
		for (j = 0; j < img->w; ++j)
		{
			if (!get_pixel(img, i, j))
				continue;

			/* Если хотя бы один из 4-соседей — белый (или нет),
			 * то этот пиксель — граница.
			 */
			if (!get_pixel(img, i - 1, j)
				|| !get_pixel(img, i + 1, j)
				|| !get_pixel(img, i, j - 1)
				|| !get_pixel(img, i, j + 1))
			{
				++perim;
			}
		}
	}

	img->perim = perim;
}

/**
 * Рекурсивно заливает компонент связности (4-связность),
 * помечая посещённые пиксели значением 2 во временной карте.
 *
 * @param data   Копия битовой карты (модифицируется).
 * @param w,h    Размеры.
 * @param i,j    Стартовая позиция.
 * @param rb     Количество байтов на строку.
 */
static void	flood_fill(unsigned char *data, int w, int h,
				int i, int j, int rb)
{
	if (i < 0 || i >= h || j < 0 || j >= w)
		return;

	/* Проверяем, чёрный ли пиксель (бит = 1) и не посещён ли */
	if (!(ISBIT((7 - j % 8), data[i * rb + j / 8])))
		return;

	/* Помечаем как посещённый — сбрасываем бит */
	data[i * rb + j / 8] &= (unsigned char)~(1 << (7 - j % 8));

	/* Рекурсивно обходим 4-соседей */
	flood_fill(data, w, h, i - 1, j, rb);
	flood_fill(data, w, h, i + 1, j, rb);
	flood_fill(data, w, h, i, j - 1, rb);
	flood_fill(data, w, h, i, j + 1, rb);
}

/**
 * Вычисляет связность глифа (количество 4-связных компонентов).
 *
 * Использует flood fill на копии битовой карты.
 * Каждый запуск flood fill обнуляет один компонент.
 */
static void	calc_connectivity(IMG *img)
{
	unsigned char	*copy;
	int				rb;
	int				i, j;
	int				components;

	rb = row_bytes(img);
	copy = (unsigned char *)malloc((size_t)img->bytes);
	if (copy == NULL)
	{
		img->conn = -1;
		return;
	}

	memcpy(copy, img->data, (size_t)img->bytes);

	components = 0;
	for (i = 0; i < img->h; ++i)
	{
		for (j = 0; j < img->w; ++j)
		{
			if (ISBIT((7 - j % 8), copy[i * rb + j / 8]))
			{
				++components;
				flood_fill(copy, img->w, img->h, i, j, rb);
			}
		}
	}

	free(copy);
	img->conn = components;
}

/**
 * Вычисляет все характеристики глифа.
 */
static void	analyze_glyph(IMG *img)
{
	calc_count_and_density(img);
	calc_diameter(img);
	calc_perimeter(img);
	calc_connectivity(img);
}

/*
 * ---------------------------------------------------------------
 * Поиск похожих глифов
 * ---------------------------------------------------------------
 */

/**
 * Сравнивает два глифа одинакового размера и подсчитывает
 * количество различающихся пикселей.
 *
 * @param  a, b  Указатели на глифы (должны быть a->w == b->w, a->h == b->h).
 * @return       Количество различающихся пикселей.
 */
static int	count_diff_pixels(const IMG *a, const IMG *b)
{
	int				i;
	int				diff;
	int				bit;
	unsigned char	xor_byte;
	diff = 0;

	for (i = 0; i < a->bytes; ++i)
	{
		xor_byte = (unsigned char)(a->data[i] ^ b->data[i]);
		if (xor_byte == 0)
			continue;

		/* Считаем установленные биты в xor_byte */
		for (bit = 7; bit >= 0; --bit)
		{
			if (ISBIT(bit, xor_byte))
				++diff;
		}
	}

	return (diff);
}

/**
 * Ищет похожие глифы среди глифов одинакового размера.
 *
 * Два глифа считаются похожими, если количество различающихся
 * пикселей не превышает 10% от общего количества пикселей.
 *
 * Результат выводится в stdout.
 */
static void	find_similar_glyphs(void)
{
	int		i, j;
	int		total_pixels;
	int		diff;
	int		max_diff;
	int		similar_count;

	printf("\n=== ПОИСК ПОХОЖИХ ГЛИФОВ ===\n");
	printf("(не более 10%% различающихся пикселей)\n\n");

	similar_count = 0;

	for (i = 0; i < N; ++i)
	{
		for (j = i + 1; j < N; ++j)
		{
			/* Только глифы одинакового размера */
			if (G[i]->w != G[j]->w || G[i]->h != G[j]->h)
				continue;

			total_pixels = G[i]->w * G[i]->h;
			max_diff = total_pixels / 10;	/* 10% */

			diff = count_diff_pixels(G[i], G[j]);
			if (diff <= max_diff)
			{
				printf("Похожи: глиф #%d (id=%d) и глиф #%d (id=%d)"
					   " | размер %dx%d | различается %d пикс. (%.1f%%)\n",
					   i, G[i]->id, j, G[j]->id,
					   G[i]->w, G[i]->h,
					   diff, (double)diff * 100.0 / (double)total_pixels);
				++similar_count;
			}
		}
	}

	if (similar_count == 0)
		printf("Похожих глифов не найдено.\n");
	else
		printf("\nВсего найдено %d пар похожих глифов.\n", similar_count);
}

/*
 * ---------------------------------------------------------------
 * Вывод результатов
 * ---------------------------------------------------------------
 */

/**
 * Выводит информацию о глифе.
 */
static void	print_glyph_info(int idx, const IMG *img)
{
	printf("Глиф #%d (id=%d): %dx%d, чёрных=%d, плотность=%.4f, "
		   "диаметр=%d, периметр=%d, связность=%d\n",
		   idx, img->id, img->w, img->h,
		   img->count, img->density,
		   img->diam, img->perim, img->conn);
}

/*
 * ---------------------------------------------------------------
 * Загрузка всех глифов из каталога
 * ---------------------------------------------------------------
 */

/**
 * Загружает все глифы из указанного каталога.
 *
 * Имена файлов должны начинаться с "glif_".
 *
 * @param  dirname Имя каталога.
 * @return         Количество загруженных глифов, или -1 при ошибке.
 */
static int	load_all_glyphs(const char *dirname)
{
	DIR				*d;
	struct dirent	*entry;
	char			path[PATH_MAX];
	int				id;

	d = opendir(dirname);
	if (d == NULL)
	{
		fprintf(stderr, "Ошибка: не удалось открыть каталог \"%s\".\n", dirname);
		return (-1);
	}

	N = 0;
	id = 0;

	while ((entry = readdir(d)) != NULL)
	{
		/* Пропускаем . и .. */
		if (entry->d_name[0] == '.')
			continue;

		/* Проверяем префикс "glif_" */
		if (strncmp(entry->d_name, "glif_", 5) != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

		G[N] = load_img(path);
		if (G[N] == NULL)
		{
			fprintf(stderr, "Ошибка: не удалось загрузить \"%s\".\n", path);
			continue;
		}

		printf("Загружен: %s (id=%d, %dx%d)\n",
			   entry->d_name, G[N]->id, G[N]->w, G[N]->h);

		/* Заменяем id на порядковый номер, если нужно */
		G[N]->id = id;
		++id;
		++N;
	}

	closedir(d);
	return (N);
}

/*
 * ---------------------------------------------------------------
 * Освобождение памяти
 * ---------------------------------------------------------------
 */
static void	cleanup(void)
{
	for (int i = 0; i < N; ++i)
	{
		free(G[i]->data);
		free(G[i]);
	}
}

/*
 * ---------------------------------------------------------------
 * Точка входа
 * ---------------------------------------------------------------
 */
/*
 * Сборка: gcc -Wall -Wextra -g3 main.c -o output/main
 * Запуск: ./output/main glyph/
 */

int	main(int argc, char *argv[])
{
	int	i;

	if (argc != 2)
	{
		fprintf(stderr, "Использование: %s <каталог_с_глифами>\n", argv[0]);
		fprintf(stderr, "Пример: %s glyph/\n", argv[0]);
		return (1);
	}

	printf("Загрузка глифов из каталога '%s'...\n\n", argv[1]);

	if (load_all_glyphs(argv[1]) <= 0)
	{
		fprintf(stderr, "Глифы не найдены в каталоге '%s'.\n", argv[1]);
		return (1);
	}

	printf("\n=== АНАЛИЗ ГЛИФОВ ===\n\n");

	for (i = 0; i < N; ++i)
	{
		analyze_glyph(G[i]);
		print_glyph_info(i, G[i]);
	}

	find_similar_glyphs();

	cleanup();
	return (0);
}

