// TODO: Reorganize this
char TexturedQuadShader[] = R"FOO(

struct vs {
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

ps vsf(vs input) {
	ps result;

	result.pixel_pos = mul(view_proj, float4(input.position, 1.0));
	result.texcoord = input.texcoord;

	return result;
}

Texture2D diffuse_texture : register(t0);
SamplerState default_sampler : register(s0);

float4 psf(ps input) : SV_TARGET {
	float3 diffuse_color = diffuse_texture.Sample(default_sampler, input.texcoord).xyz;
	return float4(diffuse_color, 1.0f);
}
	)FOO";

char QuadShader[] = R"FOO(

struct Quad {
	float3 tl;
	float3 tr;
	float3 bl;
	float3 br;
	float4 color;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

StructuredBuffer<Quad> quad_list : register(t0);

ps vsf(in uint vert_id : SV_VertexID) {
	ps result;
	int index = vert_id / 6;
	int vert_count = vert_id % 6;

	Quad quad = quad_list[index];

	float3 corner;
	if(vert_count == 0)
		corner = quad.tl;
	if(vert_count == 1)
		corner = quad.bl;
	if(vert_count == 2)
		corner = quad.br;

	if(vert_count == 3)
		corner = quad.br;
	if(vert_count == 4)
		corner = quad.tr;
	if(vert_count == 5)
		corner = quad.tl;

	float4 pos = float4(corner, 1.0);
	result.pixel_pos = mul(view_proj, pos);

	result.color = quad.color;
	return result;
}

float4 psf(ps input) : SV_TARGET {
	return input.color;
}
	)FOO";

char CipSpaceTexturedShader[] = R"FOO(
	
struct vs {
	float3 position: POSITION;
	float2 texcoord: TEXCOORD;
};

struct ps {
	float4 pixel_pos: SV_POSITION;
	float2 texcoord: TEXCOORD;
};

ps vsf(vs input) {
	ps result;

	result.position = float4(input.position, 1.0f);
	result.texcoord = input.texcoord;

	return result;
};

)FOO";


char ScreenSpaceShader[] = R"FOO(

struct vs {
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
};

cbuffer screen : register(b0) {
	float width;
	float height;
};

struct ps {
	float4 pixel_pos: SV_POSITION;
	float2 texcoord: TEXCOORD;
};

ps vsf(vs input) {
	ps result;

	float x_norm = input.position.x/width;
	float y_norm = input.position.y/height;

	//float x = x_norm*2 - 1;
	//float y = 1 - y_norm*2;

	//result.pixel_pos = float4(x, y, input.position.z, 1); 
	result.pixel_pos = float4(x_norm, y_norm, input.position.z, 1); 
	result.texcoord = input.texcoord;

	return result;
};

)FOO";

	char TextShader[] = R"FOO(
// NDC TopLeft(-1, 1), BottomRight(1, -1)

struct Glyph {
	float2 pos0;			// Normalized top left
	float2 pos1;  		// normalized bot right
	float2 uv0;
	float2 uv1;
	float3 color; 
};

StructuredBuffer<Glyph> glyph : register(t0);

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 color : COLOR;
};

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

	top_left = float2(     map (top_left.x     , 0 , 1 , -1 , 1) , map (top_left.y     , 0 , 1 , 1 , -1));
	top_right = float2(    map (top_right.x    , 0 , 1 , -1 , 1) , map (top_right.y    , 0 , 1 , 1 , -1));
	bottom_right = float2( map (bottom_right.x , 0 , 1 , -1 , 1) , map (bottom_right.y , 0 , 1 , 1 , -1));
	bottom_left = float2(  map (bottom_left.x  , 0 , 1 , -1 , 1) , map (bottom_left.y  , 0 , 1 , 1 , -1));

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
)FOO";


char MonochromeShader[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

Texture2D diffuse_texture : register(t0);
SamplerState default_sampler : register(s0);

float4 psf(ps input) : SV_Target {
	float4 diffuse_color = diffuse_texture.Sample(default_sampler, input.texcoord).x;
	return diffuse_color;
}

)FOO";

char MeshShader[] = R"FOO(

struct MeshInfo {
	float4x4 model;
	float4 color;
};

struct vs {
	float3 position : POSITION;
	float3 normal : NORMAL;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

cbuffer mesh : register(b2) {
	float4x4 model;
	float4 color;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float3 vertex_pos : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
};

cbuffer light : register(b1) {
	float3 light_position;
	float light_ambience;
};

ps vsf(vs input, in uint instance_id : SV_InstanceID) {
	ps output;
	
	float4 world_pos = mul(model, float4(input.position, 1.0f));
	output.pixel_pos = mul(view_proj, world_pos);
	output.vertex_pos = world_pos.xyz;
	output.normal = mul(model, float4(input.normal, 0.0)).xyz;
	output.color = color;

	return output;
}

float4 psf(ps input) : SV_Target {
	float3 light_dir = normalize(input.vertex_pos - light_position);

	float cos = max(0.0, dot(input.normal, light_dir));

	float3 diffuse_color = cos * input.color.rgb;
	float3 ambient_color = input.color.rbg * light_ambience;

	float3 final_color = ambient_color + diffuse_color;

	return float4(final_color, input.color.a);
};
	)FOO";

char FullScreenQuadShader[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

ps vsf(in uint vert_id : SV_VertexID) {
	ps result;

	result.texcoord = float2((vert_id << 1) & 2, vert_id & 2);
	result.pixel_pos = float4(result.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

	return result;
}
)FOO";

char CopyShader[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

Texture2D tex: register(t0);
SamplerState tex_sampler : register(s0);


)FOO";

char PostProcessShader[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

#define MAX_SAMPLES 16

cbuffer image : register(b0) {
	float width;
	float height;
};

Texture2D tex: register(t0);
SamplerState tex_sampler : register(s0);

float4 ps_copy(ps input) : SV_Target {
	return float4(tex.Sample(tex_sampler, input.texcoord).xyz, 1.0);
};

float4 ps_edge(ps input) : SV_Target {
	float2 uv = input.texcoord;

	float4 x_acc = 0;
	float4 y_acc = 0;
	float2 texel = float2(1/width, 1/height);

	x_acc += tex.Sample(tex_sampler, uv + float2(-texel.x, -texel.y))  * -1.0;
	x_acc += tex.Sample(tex_sampler, uv + float2(-texel.x,  			0))  * -2.0;
	x_acc += tex.Sample(tex_sampler, uv + float2(-texel.x,  texel.y))  * -1.0;

	x_acc += tex.Sample(tex_sampler, uv + float2( texel.x, -texel.y))  *  1.0;
	x_acc += tex.Sample(tex_sampler, uv + float2( texel.x,  			0))  *  2.0;
	x_acc += tex.Sample(tex_sampler, uv + float2( texel.x,  texel.y))  *  1.0;

	y_acc += tex.Sample(tex_sampler, uv + float2(-texel.x, -texel.y))  * -1.0;
	y_acc += tex.Sample(tex_sampler, uv + float2(       0, -texel.y))  * -2.0;
	y_acc += tex.Sample(tex_sampler, uv + float2( texel.x, -texel.y))  * -1.0;

	y_acc += tex.Sample(tex_sampler, uv + float2(-texel.x,  texel.y))  *  1.0;
	y_acc += tex.Sample(tex_sampler, uv + float2(       0,  texel.y))  *  2.0;
	y_acc += tex.Sample(tex_sampler, uv + float2( texel.x,  texel.y))  *  1.0;

	return sqrt(x_acc*x_acc + y_acc*y_acc);
};

)FOO";

char UIShader[] = R"FOO(

struct UIBuffer {
	float2 p0; // normalized to screen coordinates
	float2 p1;
	float4 color[4];
};

StructuredBuffer<UIBuffer> ui_buffer_list : register(t0);

struct ps {
	float4 pixel_pos: SV_POSITION;
	float4 color: COLOR;
};

ps vsf(in uint vert_id: SV_VertexID, in uint instance_id: SV_InstanceID) {
	ps result;

	static float2 vertices[] = {
		{-1, -1},
		{-1,  1},
		{ 1, -1},
		{ 1,  1}
	};

	UIBuffer ui = ui_buffer_list[instance_id];

	float2 half_size = (ui.p1 - ui.p0)/2;
	float2 center = (ui.p1 + ui.p0)/2;
	float2 pos = vertices[vert_id] * half_size + center;

	result.pixel_pos = float4(2*pos.x - 1, 1 - 2*pos.y, 0, 1);
	result.color = ui.color[vert_id];

	return result;
};

float4 psf(ps input) : SV_Target {
	return input.color;
}

)FOO";




