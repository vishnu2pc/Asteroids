#define _pad float _ignore;

//-----------------Vertex Structures-----------------------------------------------------
struct vs {
	float3 position : POSITION;
	float3 normal : NORMAL;
};
struct vs_tex {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};
cbuffer vs_per_camera : register(b1) {
	float4x4 view_proj;
};
cbuffer vs_per_object : register(b3) {
	float4x4 model;
};

//------------------Pixel Structures------------------------------------------------------
struct ps {
	float4 pixel_coord : SV_POSITION;
	float3 vertex_pos : POSITION;
	float3 normal : NORMAL;
};
struct ps_tex {
	float4 pixel_coord : SV_POSITION;
	float3 vertex_pos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};
cbuffer ps_per_camera : register(b1) {
	float3 light_pos; 
	float ambience;
};
cbuffer ps_per_material : register(b2) {
	float3 col; 
	float diffuse_factor;
};
Texture2D albedo_texture : register(t0);
SamplerState texture_sampler : register(s0);

// -----------------Vertex Functions------------------------------
ps vsf(vs input) {
	ps output;

	float4 pos = float4(input.position, 1.0f);
	float4 world_pos = mul(model, pos);
	output.vertex_pos = world_pos.xyz;
	output.pixel_coord = mul(view_proj, world_pos);
	output.normal = normalize(mul(float4(input.normal, 0.0f), model).xyz);

	return output;
}
ps_tex vsf_tex(vs_tex input) {
	ps_tex output;

	float4 pos = float4(input.position, 1.0f);
	float4 world_pos = mul(model, pos);
	output.vertex_pos = world_pos.xyz;
	output.pixel_coord = mul(view_proj, world_pos);
	output.normal = normalize(mul(float4(input.normal, 0.0f), model).xyz);
	output.texcoord = input.texcoord;

	return output;
}
//---------------------Pixel Functions---------------------------------------------------
float4 psf(ps input, bool front_facing: SV_IsFrontFace) : SV_TARGET {
	float3 final_color;
	float3 light_dir = normalize(light_pos.xyz - input.vertex_pos);
	float3 face_sided_normal;

	if(front_facing) face_sided_normal = input.normal;
	else face_sided_normal = -input.normal;

	float cos = max(0.0, dot(light_dir, face_sided_normal));

	float3 ambient_color = ambience * col;
	float3 diffuse_color = cos * col * diffuse_factor;

	final_color = diffuse_color + ambient_color;
	return float4(final_color, 1.0f);
}
float4 psf_tex(ps_tex input, bool front_facing: SV_IsFrontFace) : SV_TARGET {
	float3 final_color;
	float3 light_dir = normalize(light_pos.xyz - input.vertex_pos);
	float3 face_sided_normal;

	if(front_facing) face_sided_normal = input.normal;
	else face_sided_normal = -input.normal;

	float cos = max(0.0, dot(light_dir, face_sided_normal));

	float3 ambient_color = ambience * col;
	//float3 diffuse_color = cos * col * diffuse_factor * albedo_texture.Sample(texture_sampler, input.texcoord).xyz;
	float3 diffuse_color = albedo_texture.Sample(texture_sampler, input.texcoord).xyz;
	return float4(diffuse_color, 1.0);

	final_color = diffuse_color + ambient_color;
	return float4(final_color, 1.0f);
}