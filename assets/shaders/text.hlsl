// NDC TopLeft(-1, 1), BottomRight(1, -1)

struct Glyph {
	float2 pos;							// Normalized between 0 and 1, top left being (0,0), bottom right is (1, 1)
	float2 size_of_bb;  		// Same
	int2 size_of_tex;				// Pixel coords
	int2 offset_into_tex;		// Same
	float3 color; 
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	int2 texcoord : TEXCOORD;
	float3 color : COLOR;
};

StructuredBuffer<Glyph> glyph : register(t0);

float map(float value, float min1, float max1, float min2, float max2) {
	// Convert the current value to a percentage
	// 0% - min1, 100% - max1
	float perc = (value - min1) / (max1 - min1);

	// Do the same operation backwards with min2 and max2
	float final = perc * (max2 - min2) + min2;
	return final;
}

ps vsf(in uint vert_id : SV_VertexID) {
	ps output;
	// Vertices are drawn ccw
	// 0, 1, 2, 3, 4, 5
	// 2, 0, 1, 2, 1, 3

	float posx = glyph[0].pos.x;
	float posy = glyph[0].pos.y;
	float xoff = glyph[0].size_of_bb.x;
	float yoff = glyph[0].size_of_bb.y;
	float zval = 1.0f;

	float2 top_left = float2(posx, posy);
	float2 top_right = float2(posx+xoff, posy);
	float2 bottom_right = float2(posx+xoff, posy+yoff);
	float2 bottom_left = float2(posx, posy+yoff);

	top_left = float2(map(top_left.x, 0, 1, -1, 1), map(top_left.y, 0, 1, 1, -1));
	top_right = float2(map(top_right.x, 0, 1, -1, 1), map(top_right.y, 0, 1, 1, -1));
	bottom_right = float2(map(bottom_right.x, 0, 1, -1, 1), map(bottom_right.y, 0, 1, 1, -1));
	bottom_left = float2(map(bottom_left.x, 0, 1, -1, 1), map(bottom_left.y, 0, 1, 1, -1));

	/*
	posx = posx/1600.0f;
	posy = posy/900.0f;
	xoff = xoff/1600.0f;
	yoff = xoff/900.0f;
	*/

	if(vert_id == 0)
	output.pixel_pos = float4(top_left, zval, zval);
	if(vert_id == 1)
		output.pixel_pos = float4(bottom_left, zval, zval);
	if(vert_id == 2)
		output.pixel_pos = float4(bottom_right, zval, zval);
	if(vert_id == 3)
		output.pixel_pos = float4(bottom_right, zval, zval);
	if(vert_id == 4)
		output.pixel_pos = float4(top_right, zval, zval);
	if(vert_id == 5)
		output.pixel_pos = float4(top_left, zval, zval);

	output.color = glyph[0].color;
	return output;
}

float4 psf(ps input) : SV_TARGET {
	return float4(input.color, 1.0);
}
