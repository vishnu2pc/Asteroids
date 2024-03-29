struct Line{
	float3 start;
	float3 end;
	float3 color;
};

cbuffer vs_per_camera : register(b1) {
	float4x4 view_proj;
}

struct ps {
	float4 pixel_coord : SV_POSITION;
	float3 color : COLOR;
};

StructuredBuffer<Line> line: register(t0);

ps vsf(in uint vert_id : SV_VertexID) {
	ps output;

	int index = vert_id/2;
	int vert_count = vert_id % 2;

	Line line = line_array[index];

	float3 line_end_to_draw = lerp(line.start, line.end, vert_count);
	output.pixel_coord = mul(view_proj, float4(line_end_to_draw, 1.0f));

	output.color = line.color;
	return output;
}

float4 psf(ps input) : SV_TARGET {
	return float4(input.color, 1.0f);
}
