#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
unsigned char ttf_buffer[1 << 25];

// Start and end codepoint for common ascii chars
#define FONT_CHARS_TOTAL 0x7E - 0x20

// File header
// No packing
struct FontInfo {
	unsigned short height_of_char_in_bitmap;	
	unsigned short ascent, descent, line_gap, baseline;	// advance vertical position by (ascent - descent + line_gap)
	stbtt_packedchar packed_chars[FONT_CHARS_TOTAL];
};
// Single channel bitmap after this struct

int main(int argc, char** argv) {
	fread(ttf_buffer, 1, 1<<25, fopen("../../../assets/fonts/JetBrainsMono/JetBrainsMono-Light.ttf", "rb"));
	stbtt_fontinfo info;
	assert(stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0)));
	{
		static stbtt_pack_context pc;
		static stbtt_packedchar cd[FONT_CHARS_TOTAL];

#define BITMAP_H 1000
#define BITMAP_W 1000

		unsigned short bitmap_w, bitmap_h;
		bitmap_h = BITMAP_H;
		bitmap_w = BITMAP_W;
		unsigned short font_height = 100.0f;
		static unsigned char atlas[BITMAP_W * BITMAP_H];

		int ascent, descent, line_gap, baseline;
		float scale_factor = stbtt_ScaleForPixelHeight(&info, font_height);
		stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap); 
		ascent = (unsigned short)(ascent * scale_factor);
		descent = (unsigned short)(descent * scale_factor);
		line_gap = (unsigned short)(line_gap * line_gap);
		baseline = (unsigned short)(ascent*scale_factor);

		FontInfo* font_info = (FontInfo*)calloc(1, sizeof(FontInfo));
		font_info->height_of_char_in_bitmap = font_height;
		font_info->ascent = ascent;
		font_info->descent = descent;
		font_info->line_gap = line_gap;
		font_info->baseline = baseline;

		stbtt_PackBegin(&pc, atlas, 1000,1000,0,1,NULL);
		stbtt_PackSetOversampling(&pc, 2, 2);
		stbtt_PackFontRange(&pc, ttf_buffer, 0, 100.0, 0x20, 0x7E-0x20, font_info->packed_chars); 
		stbtt_PackEnd(&pc);

		stbi_write_png("jetbrains_mono_light.png", bitmap_w, bitmap_h, 1, atlas, 0);

		FILE* file = fopen("jetbrains_mono_light.fi", "w");
		fwrite(font_info, sizeof(FontInfo), 1, file);
		//fwrite(atlas, sizeof(atlas), 1, file);

		fclose(file);
	}
	return 1;
}