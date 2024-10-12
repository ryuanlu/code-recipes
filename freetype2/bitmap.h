#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <ft2build.h>
#include FT_FREETYPE_H

struct bitmap;

struct bitmap* bitmap_new(const int width, const int height);
void bitmap_destroy(struct bitmap* bitmap);
void bitmap_show(struct bitmap* bitmap);
void bitmap_draw_glyph(struct bitmap* bitmap, FT_GlyphSlot slot, int p_x, int p_y);



#endif /* __BITMAP_H__ */