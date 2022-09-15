#define MAX_DEBUG_TEXT_GLYPHS 1000

enum QUADRANT {
	QUADRANT_TOP_LEFT,
	QUADRANT_TOP_RIGHT,
	QUADRANT_BOTTOM_LEFT,
	QUADRANT_BOTTOM_RIGHT,
	QUADRANT_TOTAL
};

struct PackedChar {
	u16 x0, y0, x1, y1;
	float xoff, yoff, xadvance;
	float xoff2, yoff2;
};

#define FONT_CHARS_TOTAL (0x7E - 0x20)
// Stack no alloc
struct FontInfo {
	u16 height_of_char_in_bitmap;
	u16 bitmap_w, bitmap_h;
	int ascent, descent, line_gap, baseline;
	PackedChar pc[FONT_CHARS_TOTAL];
};

struct GlyphQuad {
	float x0, y0;
	float x1, y1;
	float u0, v0;
	float u1, v1;
	Vec3 color;
};
