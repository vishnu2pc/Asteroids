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
	float4 pixel_position : SV_POSITION;
	float3 vertex_position : POSITION;
	float3 normal : NORMAL;
};
cbuffer pixel_per_camera : register(b1) {
	float3 light_position;
	float3 ambience;
};
cbuffer pixel_per_object : register(b2) {
	float3 color;
};
// -----------------------------------------------
ps_in vs_main(vs_in input) {
	ps_in output;

	float4x4 mvp = mul(view_proj, model);

	output.pixel_position = mul(mvp, float4(input.position, 1.0f));
	output.normal = normalize(mul(model, float4(input.normal, 0.0f)).xyz);
	output.vertex_position = mul(mvp, float4(input.position, 1.0f)).xyz;

	return output;
}
//------------------------------------------------------------------------
float4 ps_main(ps_in input) : SV_TARGET {
	float3 camera_direction = normalize(light_position - input.vertex_position);

	float cos = max(0.0f, dot(light_position, input.normal));

	float3 final_color = ambience * color * cos;
	return float4(final_color, 1.0);
}