#define _A16 __declspec(align(16))

struct DiffusePC {
	_A16 Vec3 light_position;
	_A16 float ambience;
};