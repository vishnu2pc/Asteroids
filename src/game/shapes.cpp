struct Quad {
	Vec3 tl;
	Vec3 tr;
	Vec3 bl;
	Vec3 br;
};

struct Line {
	Vec3 start;
	Vec3 end;
};

static Quad
MakeQuadFromLine(Line* line, float thickness, Vec3 normal) {
	Quad result = {};

	result.tl = V3Add(line->start, V3MulF(normal, thickness));
	result.bl = V3Add(line->start, V3MulF(normal, -thickness));
	result.tr = V3Add(line->end, V3MulF(normal, thickness));
	result.br = V3Add(line->end, V3MulF(normal, -thickness));
	return result;
}

// a, b, c, d are coordinates on an unit sphere
// Has 12 vertices, 4 unique
static void
GenerateTetrahedron(Vec3* out_vertices) {
	float a = 1.0f / 3.0f;
	float b = sqrtf(8.0f / 9.0f);
	float c = sqrtf(2.0f / 9.0f);
	float d = sqrtf(2.0f / 3.0f);

	Vec3 vertices[4] = { V3(0.0, 0.0, 1.0f), 
											 V3(-c, d, -a),	
											 V3(-c, -d, -d),
											 V3(b, 0, -a) };

	out_vertices[0] = vertices[0];
	out_vertices[1] = vertices[1];
	out_vertices[2] = vertices[2];
	out_vertices[3] = vertices[0];
	out_vertices[4] = vertices[2];
	out_vertices[5] = vertices[3];
	out_vertices[6] = vertices[0];
	out_vertices[7] = vertices[3];
	out_vertices[8] = vertices[1];
	out_vertices[9] = vertices[3];
	out_vertices[10] = vertices[2];
	out_vertices[11] = vertices[1];
}
