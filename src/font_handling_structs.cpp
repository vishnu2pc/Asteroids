#define MAX_GLYPHS_ON_SCREEN 6

struct PackedChar {
	u16 x0, y0, x1, y1;
	float xoff, yoff, xadvance;
	float xoff2, yoff2;
};

#define FONT_CHARS_TOTAL (0x7E - 0x20)
// Stack no alloc
struct FontInfo {
	u16 height_of_char_in_bitmap;
	u16 ascent, descent, line_gap, baseline;
	PackedChar pc[FONT_CHARS_TOTAL];
};

struct Glyph {
	float posx, posy;
	float bb_size_x, bb_size_y;
	int tex_size_x, tex_size_y;
	int tex_off_x, tex_off_y;
	Vec3 color;
};