struct vs_in {
	float3 position : POSITION;
	float3 normal : NORMAL;
};
cbuffer vertex_per_camera : register(b1) {
	float4x4 view_proj;
};
cbuffer vertex_per_object : register(b2) {
	float4x4 model;
};
// -----------------------------------------------
struct ps_in {
	float4 pixel_coord : SV_POSITION;
	float3 vertex_pos : POSITION;
	float3 normal : NORMAL;
};
cbuffer pixel_per_camera : register(b1) {
	float4 light_pos;
	float ambience;
};
cbuffer pixel_per_object : register(b2) {
	float3 col;
};
// -----------------------------------------------
ps_in vs_main(vs_in input) {
	ps_in output;

	float4x4 mvp = mul(view_proj, model);

	output.vertex_pos = mul(model, float4(input.position, 1.0f)).xyz;
	output.pixel_coord = mul(mvp, float4(input.position, 1.0f));
	output.normal = normalize(mul(model, float4(input.normal, 0.0f)).xyz);
	return output;
}
//------------------------------------------------------------------------
float4 ps_main(ps_in input) : SV_TARGET {
	float3 final_color;
	float3 light_dir = normalize(light_pos.xyz - input.vertex_pos);
	float cos = max(0.0, dot(light_dir, input.normal));

	float3 ambient_color = ambience * col;
	float3 diffuse_color = cos * col;

	final_color = diffuse_color + ambient_color;
	return float4(final_color, 1.0f);
}