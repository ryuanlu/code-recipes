#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "bitmap.h"
#include "unicode.h"

#define CANVAS_WIDTH	(80)
#define CANVAS_HEIGHT	(44)

#define FONT_FILE	"/usr/share/fonts/opentype/source-han-sans/JP/SourceHanSansJP-Normal.otf"
#define DEFAULT_DPI	(96)

#define NR_DECIMAL_BITS	(6)
#define DEFAULT_FONT_SIZE	(12)
#define DEFAULT_STRING		"日本語助かる"

int main(int argc, char** argv)
{
	FT_Library library = NULL;
	FT_Face face = NULL;
	FT_GlyphSlot slot = NULL;
	FT_Error result = 0;

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	struct bitmap* bitmap = bitmap_new(w.ws_col, (w.ws_row - 2) * 2);
	struct ustring* string = ustring_new_from_utf8(argv[1] ? argv[1] : DEFAULT_STRING);

	result = FT_Init_FreeType(&library);

	if(result)
		goto error_init;

	result = FT_New_Face(library, FONT_FILE, 0, &face);
	if(result)
		goto error_face;

	slot = face->glyph;

	FT_Set_Char_Size(face, 0, DEFAULT_FONT_SIZE << NR_DECIMAL_BITS, DEFAULT_DPI, DEFAULT_DPI);

	int x = 0;
	int y = 20;

	for(int i = 0;i < ustring_len(string);++i)
	{
		FT_Load_Char(face, ustring_get_code_at(string, i), FT_LOAD_RENDER);
		bitmap_draw_glyph(bitmap, slot, x + slot->bitmap_left, y - slot->bitmap_top);

		x += slot->advance.x >> NR_DECIMAL_BITS;
		y += slot->advance.y >> NR_DECIMAL_BITS;
	}

	bitmap_show(bitmap);

	FT_Done_Face(face);
error_face:
	FT_Done_FreeType(library);
error_init:
	bitmap_destroy(bitmap);
	ustring_destroy(string);

	return EXIT_SUCCESS;
}