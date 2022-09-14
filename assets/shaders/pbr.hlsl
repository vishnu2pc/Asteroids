// HLSL operates on matrices, column major by default
static float3 FRESNEL_DIELECTRIC = 0.04;
static float EPSILON = 0.0001;
static float PI = 3.141592;

cbuffer vertex_per_camera : register(b1) {
	float4x4 view_proj;
}

cbuffer vertex_per_object : register(b2) {
	float4x4 model;
};

// TODO: Check if registers can have be used for different cb for different shaders
cbuffer pixel_per_camera : register(b1) {
	float4 light_direction;
	float4 light_radiance;
	float4 camera_position;
};

// RESEARCH: Is tangent always float4?
struct vs_in
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float2 texcoord : TEXCOORD;
};

struct vs_out
{
	// RESEARCH: Does SV_POSITION interp between values at different vertices
	// TODO: try this without SV
	float4 pixel_position   : SV_POSITION;
	float3 vertex_position  : POSITION;
	float2 texcoord 			  : TEXCOORD;
	float3x3 tangent_basis   : TANGENT_BASIS;
};

Texture2D    normal_texture : register(t0);
Texture2D    albedo_texture : register(t1);
Texture2D    rougness_metallic_texture : register(t2);

SamplerState mysampler : register(s0);

// TODO: Replace this with more understandable variable name
float3 FresnelSchlickApprox(float3 fresnel_at_normal_incidence, float costheta) {
	return fresnel_at_normal_incidence + (1.0 - fresnel_at_normal_incidence) * pow(1.0 - costheta, 5.0);
}

// Trowbridge and Reitz
float NDGGX(float cos_light_half, float roughness) {
	float alpha   = roughness * roughness;
	float alpha_squared = alpha * alpha;

	float denom = (cos_light_half * cos_light_half) * (alpha_squared - 1.0) + 1.0;
	return alpha_squared / (PI * denom * denom);
}

float GASchlickG1(float costheta, float k) {
	return costheta / (costheta * (1.0 - k) + k);
}

float GASchlickGGX(float cos_light_in, float cos_light_out, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; 
	return GASchlickG1(cos_light_in, k) * GASchlickG1(cos_light_out, k);
}

vs_out vs_main(vs_in input) {

	vs_out output;

	float4x4 mvp = mul(view_proj, model);
	output.texcoord = input.texcoord;
	output.pixel_position = mul(mvp, float4(input.position, 1.0f));
	output.vertex_position = mul(model, float4(input.position, 1.0f)).xyz;

	float3 binormal = cross(input.normal, input.tangent.xyz);

	float3x3 TBN_in_row_major = float3x3(input.tangent.xyz, binormal, input.normal);
	float3x3 TBN = transpose(TBN_in_row_major);	

	output.tangent_basis = mul((float3x3)model, TBN);

	return output;
}

float4 ps_main(vs_out input) : SV_TARGET {

	//return float4(light_direction, 1.0f);
	//TODO: switch to albedo everywhere
	float3 albedo = albedo_texture.Sample(mysampler, input.texcoord).xyz;
	float3 normal_in_tangent_space_unmapped = normal_texture.Sample(mysampler, input.texcoord).xyz;
	float3 metallic_rougness = rougness_metallic_texture.Sample(mysampler, input.texcoord).xyz;
	// TODO: are they always y and z coordinates?
	float roughness = metallic_rougness.y;
	float metalness = metallic_rougness.z;
	//return float4(albedo, 1.0f);
	//return float4(normal_in_tangent_space_unmapped, 1.0f);

	float3 fresnel_reflectance_at_normal_incidence = lerp(FRESNEL_DIELECTRIC, albedo, metalness);
	//return float4(fresnel_reflectance_at_normal_incidence, 1.0f);

	float3 normal_in_tangent_space = normalize(2 * normal_in_tangent_space_unmapped - 1);
  //return float4(normal_in_tangent_space, 1.0f);
	float3 normal = normalize(mul(input.tangent_basis, normal_in_tangent_space));	
	//return float4(normal, 1.0f);

	float3 light_out = normalize(camera_position - input.vertex_position);
	//return float4(light_out, 1.0f);
	float cos_light_out = max(0, dot(normal, light_out));

	float3 light_reflection = 2 * cos_light_out * normal - light_out;
	//return float4(light_reflection, 1.0f);

	float3 lighting = 0;
	{
		float3 light_in = -light_direction.xyz;
		float3 radiance = light_radiance.xyz;

		float3 light_half = normalize(light_in + light_out);
		//return float4(light_half, 1.0f);

		float cos_light_in = max(0.0, dot(normal, light_in));
		float cos_light_half = max(0.0, dot(normal, light_half));

		float cos_light_out_half = max(0.0, dot(light_half, light_out));

		float3 fresnel = FresnelSchlickApprox(fresnel_reflectance_at_normal_incidence, 
																					max(0.0f, dot(light_half, light_out)));
		//return float4(fresnel, 1.0f);
		float distribution = NDGGX(cos_light_half, roughness);
		float attentuation = GASchlickGGX(cos_light_in, cos_light_out, roughness);

		float3 kd = lerp(float3(1, 1, 1) - fresnel, float3(0, 0, 0), metalness);
		//return float4(kd, 1.0f);
		
		float3 diffuse_brdf = kd * albedo;
		float3 specular_brdf = (fresnel * distribution * attentuation) / 
													 max(EPSILON, 4 * cos_light_in * cos_light_out);
		//return float4(specular_brdf, 1.0f);

		lighting += (diffuse_brdf + specular_brdf) * light_radiance * cos_light_in;
	}
	return float4(lighting, 1.0f);
}

float4 ps_shaded(vs_out input) : SV_TARGET
{
	return float4(1, 1, 1, 1);
}
