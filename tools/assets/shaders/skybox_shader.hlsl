struct vs_in {
	float3 position : position;
}
struct vs_out {
	float3 texcoord : TEXCOORD;
}

/* TODO: Separate view_proj from camera */
cbuffer vertex_per_camera : register(b1) {
	float4x4 view_proj;
	float4x4 perspective;
}

TextureCube skybox : register(t0);
SamplerState sampler : register(s0);

vs_out vs_main(vs_in input) {
	vs_out output;

}