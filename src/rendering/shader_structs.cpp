#define _A16 __declspec(align(16))

struct DiffusePC {
	Vec3 light_position;
	float ambience;
};

struct DiffusePM {
	Vec3 color;
	float diffuse_factor;
};
