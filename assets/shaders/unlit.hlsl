cbuffer pixel_per_object : register(b2) {
	float3 color;
};

float4 ps_main() : SV_TARGET {
	return float4(color, 1.0f);
}