struct Quad {
	union {
		struct {
			Vec3 tl;
			Vec3 tr;
			Vec3 bl;
			Vec3 br;
		};
		struct {
			Vec3 max;
			Vec3 min;
		};
	};
};

struct Line {
	Vec3 start;
	Vec3 end;
};

// input assumed to be triangle list
static void 
GenerateFlatShadedNormals(Vec3* vertices, u32 count, Vec3* out_normals ) {
	Assert(count % 3 == 0);
	for(u32 i=0; i<count; i+=3) {
		Vec3 a = vertices[i];
		Vec3 b = vertices[i+1];
		Vec3 c = vertices[i+2];
		Vec3 cross = V3Norm(V3Cross(V3Sub(b, a), V3Sub(c, a)));
		out_normals[i] = cross;
		out_normals[i+1] = cross;
		out_normals[i+2] = cross;
	}
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

static bool
IsPointInQuad(Vec2 min, Vec2 max, Vec2 point) {
	bool result = 0;

	if(point.x <= max.x &&
		 point.x >= min.x &&
		 point.y <= max.y &&
		 point.y >= min.y) result = true;

	return result;
}



