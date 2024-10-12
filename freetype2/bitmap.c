#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct bitmap
{
	uint8_t*	data;
	int	width;
	int	height;

};


struct bitmap* bitmap_new(const int width, const int height)
{
	struct bitmap* bitmap = NULL;

	bitmap = calloc(1, sizeof(struct bitmap));

	bitmap->data = calloc(1, 4 * width * height);
	bitmap->width = width;
	bitmap->height = height;

	return bitmap;
}

void bitmap_destroy(struct bitmap* bitmap)
{
	free(bitmap->data);
	free(bitmap);
}


void bitmap_show(struct bitmap* bitmap)
{
	for(int y = 0;y < (bitmap->height + 1) / 2;++y)
	{
		for(int x = 0;x < bitmap->width;++x)
		{
			int upper[4];
			int lower[4];
			
			for(int i = 0;i < 4;++i)
				upper[i] = bitmap->data[y * bitmap->width * 8 + 4 * x + i];

			if((y * 2 + 1) < bitmap->height)
			{
				for(int i = 0;i < 4;++i)
					lower[i] = bitmap->data[(y * 2 + 1) * bitmap->width * 4 + 4 * x + i];
			}
			else
			{
				for(int i = 0;i < 4;++i)
					lower[i] = 0;
			}

			fprintf(stderr, "\x1b[38;2;%d;%d;%dm", lower[0], lower[1], lower[2]);
			fprintf(stderr, "\x1b[48;2;%d;%d;%dm", upper[0], upper[1], upper[2]);
			fprintf(stderr, "▄\x1b[0m");

		}
		fputs("\n", stderr);
	}
}

void bitmap_draw_glyph(struct bitmap* bitmap, FT_GlyphSlot slot, int p_x, int p_y)
{
	for(int y = 0;y < slot->bitmap.rows;++y)
	{
		for(int x = 0;x < slot->bitmap.width;++x)
		{
			if((p_y + y) < 0 || (p_x + x) < 0)
				continue;

			if((p_y + y) >= bitmap->height || (p_x + x) >= bitmap->width)
				continue;


			for(int i = 0;i < 3;++i)
				bitmap->data[(p_y + y) * bitmap->width * 4 + (p_x + x) * 4 + i] = slot->bitmap.buffer[y * slot->bitmap.pitch + x];
			
			bitmap->data[(p_y + y) * bitmap->width * 4 + (p_x + x) * 4 + 3] = 255;

		}
	}
}
