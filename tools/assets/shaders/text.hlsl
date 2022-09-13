// NDC TopLeft(-1, 1), BottomRight(1, -1)

struct Glyph {
	float2 pos0;			// Normalized top left
	float2 pos1;  		// normalized bot right
	float2 uv0;
	float2 uv1;
	float3 color; 
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
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
	int index = vert_id/6;

	// TODO: we have actual coords instead of bb size, redo this, possibly never
	float pos0x = glyph[index].pos0.x;
	float pos0y = glyph[index].pos0.y;
	float xoff = glyph[index].pos1.x - pos0x;
	float yoff = glyph[index].pos1.y - pos0y;
	float2 uv0 = glyph[index].uv0;
	float2 uv1 = glyph[index].uv1;
	float zval = 1.0f;

	float2 top_left = float2(pos0x, pos0y);
	float2 top_right = float2(pos0x+xoff, pos0y);
	float2 bottom_right = float2(pos0x+xoff, pos0y+yoff);
	float2 bottom_left = float2(pos0x, pos0y+yoff);

	top_left = float2(map(top_left.x, 0, 1, -1, 1), map(top_left.y, 0, 1, 1, -1));
	top_right = float2(map(top_right.x, 0, 1, -1, 1), map(top_right.y, 0, 1, 1, -1));
	bottom_right = float2(map(bottom_right.x, 0, 1, -1, 1), map(bottom_right.y, 0, 1, 1, -1));
	bottom_left = float2(map(bottom_left.x, 0, 1, -1, 1), map(bottom_left.y, 0, 1, 1, -1));

	int vert_count = vert_id % 6;

	if(vert_count == 0) {
		output.pixel_pos = float4(top_left, zval, zval);
		output.texcoord = uv0;
	}
	if(vert_count == 1) {
		output.pixel_pos = float4(bottom_left, zval, zval);
		output.texcoord = float2(uv0.x, uv1.y);
	}
	if(vert_count == 2) {
		output.pixel_pos = float4(bottom_right, zval, zval);
		output.texcoord = uv1;
	}
	if(vert_count == 3) {
		output.pixel_pos = float4(bottom_right, zval, zval);
		output.texcoord = uv1;
	}
	if(vert_count == 4) {
		output.pixel_pos = float4(top_right, zval, zval);
		output.texcoord = float2(uv1.x, uv0.y);
	}
	if(vert_count == 5) {
		output.pixel_pos = float4(top_left, zval, zval);
		output.texcoord = uv0;
	}

	output.color = glyph[index].color;
	return output;
}

Texture2D atlas : register(t0);
SamplerState texture_sampler : register(s0);

float4 psf(ps input) : SV_TARGET {
	float3 tex = atlas.Sample(texture_sampler, input.texcoord).xyz;
	if(tex.r == 0.0) discard;
	tex = input.color;
	return float4(tex, 1.0f);
}
