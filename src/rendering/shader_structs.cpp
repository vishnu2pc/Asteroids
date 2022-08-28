#define _A16 __declspec(align(16))

struct CameraLitDiffusePC {
	_A16 Vec3 camera_position;
	_A16 Vec3 ambience;
};

struct DiffusePC {
	_A16 Vec3 light_position;
	_A16 Vec3 ambience;
};