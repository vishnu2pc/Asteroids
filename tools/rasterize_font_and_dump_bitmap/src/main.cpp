#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
unsigned char ttf_buffer[1 << 25];
#define BITMAP_W  256
#define BITMAP_H  512
unsigned char temp_bitmap[BITMAP_H][BITMAP_W];
stbtt_bakedchar cdata[256*2]; // ASCII 32..126 is 95 glyphs
stbtt_packedchar pdata[256*2];

struct TexCoord {
	u16 x0, y0, x1, y1;
};

int main(int argc, char** argv) {
	// TODO: put the exe in system environment
	// TODO: get path to font and dump it

	//   Improved 3D API (more shippable):
	//           #include "stb_rect_pack.h"           -- optional, but you really want it
	//           stbtt_PackBegin()
	//           stbtt_PackSetOversampling()          -- for improved quality on small fonts
	//           stbtt_PackFontRanges()               -- pack and renders
	//           stbtt_PackEnd()
	//           stbtt_GetPackedQuad()
	//
	
	//   "Load" a font file from a memory buffer (you have to keep the buffer loaded)
	//           stbtt_InitFont()
	//           stbtt_GetFontOffsetForIndex()        -- indexing for TTC font collections
	//           stbtt_GetNumberOfFonts()             -- number of fonts for TTC font collections
	//
	//   Render a unicode codepoint to a bitmap
	//           stbtt_GetCodepointBitmap()           -- allocates and returns a bitmap
	//           stbtt_MakeCodepointBitmap()          -- renders into bitmap you provide
	//           stbtt_GetCodepointBitmapBox()        -- how big the bitmap must be
	//
	//   Character advance/positioning
	//           stbtt_GetCodepointHMetrics()
	//           stbtt_GetFontVMetrics()
	//           stbtt_GetFontVMetricsOS2()
	//           stbtt_GetCodepointKernAdvance()

	fread(ttf_buffer, 1, 1<<25, fopen("../../../assets/fonts/JetBrainsMono/JetBrainsMono-Light.ttf", "rb"));
	//fread(ttf_buffer, 1, 1<<25, fopen("../../../assets/fonts/DejaVuSans.ttf", "rb"));
	stbtt_fontinfo info;
	assert(stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0)));
	{
		stbtt__bitmap b;
		stbtt__point p[2];
		int wcount[2] = { 2,0 };
		p[0].x = 0.2f;
		p[0].y = 0.3f;
		p[1].x = 3.8f;
		p[1].y = 0.8f;
		b.w = 16;
		b.h = 2;
		b.stride = 16;
		b.pixels = (unsigned char*)malloc(b.w*b.h);
		stbtt__rasterize(&b, p, wcount, 1, 1, 1, 0, 0, 0, 0, 0, NULL);
		for (int i=0; i < 8; ++i)
			printf("%f\n", b.pixels[i]/255.0);
	}
#if 1
	{
		static stbtt_pack_context pc;
		static stbtt_packedchar cd[256];
		static stbtt_aligned_quad aq[256];

		static unsigned char atlas[1000*1000];

		int ascent, descent, line_gap;
		stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap); 

		stbtt_PackBegin(&pc, atlas, 1000,1000,1000,1,NULL);
		stbtt_PackSetOversampling(&pc, 2, 2);
		stbtt_PackFontRange(&pc, ttf_buffer, 0, 100.0, 0x20, 0x7E-0x20, cd); 

		stbtt_PackEnd(&pc);

		stbi_write_png("out.png", 1000, 1000, 1, atlas, 0);

		FILE* file = fopen("info.fci", "w");

		for(int i=0; i<0x7E-0x20; i++) {
			fwrite(&cd[i].x0, sizeof(unsigned short), 1, file);
			fwrite(&cd[i].y0, sizeof(unsigned short), 1, file);
			fwrite(&cd[i].x1, sizeof(unsigned short), 1, file);
			fwrite(&cd[i].y1, sizeof(unsigned short), 1, file);
		}

		fclose(file);
	}
#endif
#if 0
	{
		stbtt_pack_context pc;
		stbtt_PackBegin(&pc, temp_bitmap[0], BITMAP_W, BITMAP_H, 0, 1, NULL);
		stbtt_PackFontRange(&pc, ttf_buffer, 0, 20.0, 32, 95, pdata);
		stbtt_PackFontRange(&pc, ttf_buffer, 0, 20.0, 0xa0, 0x100-0xa0, pdata);
		stbtt_PackEnd(&pc);
		stbi_write_png("fonttest2.png", BITMAP_W, BITMAP_H, 1, temp_bitmap, 0);
	}
#endif
	return 1;
}