#define Abs(a) ((a) > 0 ? (a) : -(a))
#define Mod(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))

static float DegreesToRadians(float degrees) { return degrees * (M_PI/180.0f); }

struct Vec2 {
	union {
		struct {
			float x;
			float y;
		};
		float elem[2];
	};
};

struct Vec3 {
	union {
		struct {
			float x;
			float y;
			float z;
		};
		float elem[3];
	};
};

struct Vec4 {
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			Vec3 xyz;
			float z;
		};
		float elem[4];
	};
};

struct Quat {
	union {
		struct {
			float x, y, z, w;
		};
		struct {
			Vec3 xyz;
			float z;
		};
		float elem[4];
	};
};

// Column major
struct Mat4 {
	float elem[4][4];
};

struct Transform {
	Vec3 position;
	Quat rotation;
	Vec3 scale;
};


static Vec2 V2(float x, float y) { return Vec2 { x, y }; }
static Vec2 V2I() { return Vec2 { 1.0f, 1.0f }; }
static Vec2 V2Z() { return Vec2 { 0.0f, 0.0f }; }
 
static Vec3 V3(float x, float y, float z) { return Vec3 { x, y, z }; }
static Vec3 V3I() { return Vec3 { 1.0f, 1.0f, 1.0f }; }
static Vec3 V3Z() { return Vec3 { 0.0f, 0.0f, 0.0f }; }
static Vec3 V3Up() { return Vec3 { 0.0f, 1.0f, 0.0f }; }
static Vec3 V3Right() { return Vec3 { 1.0f, 0.0f, 0.0f }; }
static Vec3 V3Forward() { return Vec3 { 0.0f, 0.0f, -1.0f }; }

static Vec4 V4(float x, float y, float z, float w) { return Vec4 { x, y, z, w }; }
static Vec4 V4I() { return Vec4 { 1.0f, 1.0f, 1.0f, 1.0f }; }
static Vec4 V4Z() { return Vec4 { 0.0f, 0.0f, 0.0f, 0.0f }; }

static Quat MakeQuat(float x, float y, float z, float w) { return Quat { x, y, z, w}; }
static Quat MakeQuatFromV3(Vec3 vec) { return Quat { vec.x, vec.y, vec.z, 1 }; }
static Quat MakeQuatFromV4(Vec4 vec) { return Quat { vec.x, vec.y, vec.z, vec.w }; }
static Quat QuatI() { return Quat { 0.0f, 0.0f, 0.0f, 1.0f }; }

static Transform TransformI() { return Transform { V3Z(), QuatI(), V3I() }; }

static Vec2 V2Add(Vec2 left, Vec2 right) {
	return Vec2 { left.x + right.x, left.y + right.y };
}
static Vec2 V2Sub(Vec2 left, Vec2 right) {
	return Vec2 { left.x - right.x, left.y - right.y };
}
static Vec2 V2Mul(Vec2 left, Vec2 right) {
	return Vec2 { left.x * right.x, left.y * right.y };
}
static Vec2 V2Div(Vec2 left, Vec2 right) {
	return Vec2 { left.x/right.x, left.y/right.y };
}
static Vec2 V2AddF(Vec2 left, float scalar) {
	return Vec2 { left.x + scalar, left.y + scalar };
}
static Vec2 V2SubF(Vec2 left, float scalar) {
	return Vec2 { left.x - scalar, left.y - scalar };
}
static Vec2 V2MulF(Vec2 left, float scalar) {
	return Vec2 { left.x*scalar, left.y*scalar };
}
static Vec2 V2DivF(Vec2 left, float scalar) {
	return Vec2 { left.x/scalar, left.y/scalar };
}
static float V2Dot(Vec2 left, Vec2 right) {
	return left.x*right.x + left.y*right.y;
}

static Vec3 V3Add(Vec3 left, Vec3 right) { 
	return Vec3 { left.x + right.x, left.y + right.y, left.z + right.z };
}
static Vec3 V3Sub(Vec3 left, Vec3 right) {
	return Vec3 { left.x - right.x, left.y - right.y, left.z - right.z };
}
static Vec3 V3Mul(Vec3 left, Vec3 right) {
	return Vec3 { left.x * right.x, left.y * right.y, left.z * right.z };
}
static Vec3 V3Div(Vec3 left, Vec3 right) {
	return Vec3 { left.x / right.x, left.y / right.y, left.z / right.z };
}
static Vec3 V3AddF(Vec3 left, float scalar) {
	return Vec3 { left.x + scalar, left.y + scalar, left.z + scalar };
}
static Vec3 V3SubF(Vec3 left, float scalar) {
	return Vec3 { left.x - scalar, left.y - scalar, left.z - scalar };
}
static Vec3 V3MulF(Vec3 left, float scalar) {
	return Vec3 { left.x*scalar, left.y*scalar, left.z*scalar };
}
static Vec3 V3DivF(Vec3 left, float scalar) {
	return Vec3 { left.x/scalar, left.y/scalar, left.z/scalar };
}
static bool Vec3Equals(Vec3 left, Vec3 right) {
	return (left.x == right.x) && (left.y == right.y) && (left.z == right.z);
}
static float V3MagSquared(Vec3 vec) {
	return (vec.x*vec.x) + (vec.y*vec.y) + (vec.z*vec.z);
}
static float V3Mag(Vec3 vec) {
	return sqrtf(V3MagSquared(vec));
}
static Vec3 V3Norm(Vec3 vec) {
	Vec3 result = {};
	float mag = V3Mag(vec);
	if(mag) {
		result.x = vec.x * (1.0f / mag);
		result.y = vec.y * (1.0f / mag);
		result.z = vec.z * (1.0f / mag);
	}
	return result;
}
static Vec3 V3Cross(Vec3 left, Vec3 right) {
	return Vec3 { (left.y*right.z) - (left.z*right.y),
							(left.z*right.x) - (left.x*right.z),
							(left.x*right.y) - (left.y*right.x) };
}
static float V3Dot(Vec3 left, Vec3 right) {
	return left.x*right.x + left.y*right.y + left.z*right.z;
}

static Mat4 M4I() {
	return Mat4 { 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f };
}

static Mat4 M4Transpose(Mat4 mat) {
	Mat4 result = M4I();
	for(u8 col=0; col<4; col++) 
		for(u8 row=0; row<4; row++)
			result.elem[col][row] = mat.elem[row][col];
	return result;
}

static Mat4 M4Mul(Mat4 left, Mat4 right) {
	Mat4 result = M4I();
	for (u8 col = 0; col < 4; col++) {
		for (u8 row = 0; row < 4; row++) {
			float sum = 0;
			for (u8 k = 0; k < 4; k++) sum += left.elem[k][row] * right.elem[col][k];
			result.elem[col][row] = sum;
		}
	}
	return result;
}

static Mat4 M4MulF(Mat4 mat, float scalar) {
	Mat4 result = M4I();
	for(u8 col=0; col<4; col++)
		for(u8 row=0; row<4; row++)
			result.elem[col][row] = mat.elem[col][row] * scalar;
	return result;
}

static Vec4 M4MulV(Mat4 mat, Vec4 vec) {
	Vec4 result = {};
	for(u8 row=0; row<4; row++) {
		float sum = 0;
		for(u8 col=0; col<4; col++) sum += mat.elem[col][row] * vec.elem[col];
		result.elem[row] = sum;
	}
	return result;
}

static Mat4 M4Orthographic(float left, float right, float bottom,
																	 float top, float Near, float Far) {
	Mat4 result = M4I();

	result.elem[0][0] = 2.0f / (right-left);
	result.elem[1][1] = 2.0f / (top-bottom);
	result.elem[2][2] = 2.0f / (Near-Far);
	result.elem[3][3] = 1.0f;

	result.elem[3][0] = (left+right) / (left-right);
	result.elem[3][1] = (bottom+top) / (bottom-top);
	result.elem[3][2] = (Far+Near) / (Near-Far);

	return result;
}

static Mat4 M4Perspective(float fov, float aspect_ratio, float Near, float Far) {
	Mat4 result ={};
	float cot = 1.0f / tanf(fov * (M_PI/360.0f));
	result.elem[0][0] = cot / aspect_ratio;
	result.elem[1][1] = cot;
	result.elem[2][3] = -1.0f;
	result.elem[2][2] = (Near+Far) / (Near-Far);
	result.elem[3][2] = (2.0f*Near*Far) / (Near-Far);
	result.elem[3][3] = 0.0f;
	return result;
}

static Mat4 M4Translate(Vec3 translation) {
	Mat4 result = M4I();
	result.elem[3][0] = translation.x;
	result.elem[3][1] = translation.y;
	result.elem[3][2] = translation.z;
	return result;
}

static Mat4 M4Rotate(Vec3 axis, float angle) {
	Mat4 result = M4I();
	axis = V3Norm(axis);
	float sin = sinf(DegreesToRadians(angle));
	float cos = cosf(DegreesToRadians(angle));
	float one_minus_cos = 1.0f - cos;

	result.elem[0][0] = (axis.x*axis.x*one_minus_cos) + cos;
	result.elem[0][1] = (axis.x*axis.y*one_minus_cos) + (axis.z*sin);
	result.elem[0][2] = (axis.x*axis.z*one_minus_cos) - (axis.y*sin);

	result.elem[1][0] = (axis.y*axis.x*one_minus_cos) - (axis.z*sin);
	result.elem[1][1] = (axis.y*axis.y*one_minus_cos) + cos;
	result.elem[1][2] = (axis.y*axis.z*one_minus_cos) + (axis.x*sin);

	result.elem[2][0] = (axis.z*axis.x*one_minus_cos) + (axis.y*sin);
	result.elem[2][1] = (axis.z*axis.y*one_minus_cos) - (axis.x*sin);
	result.elem[2][2] = (axis.z*axis.z*one_minus_cos) + cos;

	return result;
}

static Mat4 M4Scale(Vec3 scale) {
	Mat4 result = M4I();
	result.elem[0][0] = scale.x;
	result.elem[1][1] = scale.y;
	result.elem[2][2] = scale.z;
	return result;
}

static Mat4 M4LookAt(Vec3 pos, Vec3 target, Vec3 Up) {
	Mat4 result = M4I();

	Vec3 F = V3Norm(V3Sub(target, pos));
	Vec3 S = V3Norm(V3Cross(F, Up));
	Vec3 U = V3Cross(S, F);

	result.elem[0][0] = S.x;
	result.elem[0][1] = U.x;
	result.elem[0][2] = -F.x;
	result.elem[0][3] = 0.0f;

	result.elem[1][0] = S.y;
	result.elem[1][1] = U.y;
	result.elem[1][2] = -F.y;
	result.elem[1][3] = 0.0f;

	result.elem[2][0] = S.z;
	result.elem[2][1] = U.z;
	result.elem[2][2] = -F.z;
	result.elem[2][3] = 0.0f;

	result.elem[3][0] = -V3Dot(S, pos);
	result.elem[3][1] = -V3Dot(U, pos);
	result.elem[3][2] = V3Dot(F, pos);
	result.elem[3][3] = 1.0f;

	return result;
}

static Quat QuatAdd(Quat left, Quat right) {
	return Quat { left.x+right.x, left.y+right.y, left.z+right.z, left.w+right.w };
}
static Quat QuatSub(Quat left, Quat right) {
	return Quat { left.x-right.x, left.y-right.y, left.z-right.z, left.w-right.w };
}
// Note : Follows composition format ie: concatenating rotations should be 
// new rotation * old rotation
static Quat QuatMul(Quat left, Quat right) { 
	Quat result = {};
	result.x = (left.x*right.w) + (left.y*right.z) - (left.z*right.y) + (left.w*right.x);
	result.y = (-left.x*right.z) + (left.y*right.w) + (left.z*right.x) + (left.w*right.y);
	result.z = (left.x*right.y) - (left.y*right.x) + (left.z*right.w) + (left.w*right.z);
	result.w = (-left.x*right.x) - (left.y*right.y) - (left.z*right.z) + (left.w*right.w);
	return result;
}
static Quat QuatMulF(Quat quat, float scalar) {
	return Quat { quat.x*scalar, quat.y*scalar, quat.z*scalar, quat.w*scalar };
}
static Quat QuatDivF(Quat quat, float scalar) {
	return Quat { quat.x/scalar, quat.y/scalar, quat.z/scalar, quat.w/scalar };
}
static float QuatDot(Quat left, Quat right) {
	return (left.x*right.x) + (left.y*right.y) + (left.z*right.z) + (left.w*right.w);
}
static Quat QuatInverse(Quat quat) {
	Quat result = { -quat.x, -quat.y, -quat.z, quat.w }; 
	result = QuatDivF(result, QuatDot(quat, quat));
	return result;
}
static Quat QuatNorm(Quat quat) {
	Quat result = {};
	float mag = sqrtf(QuatDot(quat, quat));
	result = QuatDivF(quat, mag);
	return result;
}
static Mat4 M4FromQuat(Quat quat) {
	Mat4 result = M4I();
	Quat quat_norm = QuatNorm(quat);
	float xx, yy, zz, xy, xz, yz, wx, wy, wz;
	xx = quat_norm.x * quat_norm.x;
	yy = quat_norm.y * quat_norm.y;
	zz = quat_norm.z * quat_norm.z;
	xy = quat_norm.x * quat_norm.y;
	xz = quat_norm.x * quat_norm.z;
	yz = quat_norm.y * quat_norm.z;
	wx = quat_norm.w * quat_norm.x;
	wy = quat_norm.w * quat_norm.y;
	wz = quat_norm.w * quat_norm.z;

	result.elem[0][0] = 1.0f - 2.0f*(yy+zz);
	result.elem[0][1] = 2.0f * (xy+wz);
	result.elem[0][2] = 2.0f * (xz-wy);
	result.elem[0][3] = 0.0f;

	result.elem[1][0] = 2.0f * (xy-wz);
	result.elem[1][1] = 1.0f - 2.0f*(xx+zz);
	result.elem[1][2] = 2.0f * (yz+wx);
	result.elem[1][3] = 0.0f;

	result.elem[2][0] = 2.0f * (xz+wy);
	result.elem[2][1] = 2.0f * (yz-wx);
	result.elem[2][2] = 1.0f - 2.0f*(xx+yy);
	result.elem[2][3] = 0.0f;

	result.elem[3][0] = 0.0f;
	result.elem[3][1] = 0.0f;
	result.elem[3][2] = 0.0f;
	result.elem[3][3] = 1.0f;

	return result;
}

static Quat QuatFromAxisAngle(Vec3 axis, float angle) {
	Quat result = {};
	Vec3 axis_norm = V3Norm(axis);
	float sin = sinf(angle/2.0f);
	result.xyz = V3MulF(axis_norm, sin);
	result.w = cosf(angle/2.0f);
	return result;
}

static Quat QuatFromEuler(float pitch , float yaw, float roll) {
	Quat result = {};

	float x0 = cosf(pitch*0.5f);
	float x1 = sinf(pitch*0.5f);
	float y0 = cosf(yaw*0.5f);
	float y1 = sinf(yaw*0.5f);
	float z0 = cosf(roll*0.5f);
	float z1 = sinf(roll*0.5f);

	result.x = x1*y0*z0 - x0*y1*z1;
	result.y = x0*y1*z0 + x1*y0*z1;
	result.z = x0*y0*z1 - x1*y1*z0;
	result.w = x0*y0*z0 + x1*y1*z1;

	return result;
};

static Vec3 EulerFromQuat(Quat quat) {
	Vec3 result = {};

	// Roll (x-axis rotation)
	float x0 = 2.0f*(quat.w*quat.x + quat.y*quat.z);
	float x1 = 1.0f - 2.0f*(quat.x*quat.x + quat.y*quat.y);
	result.x = atan2f(x0, x1);

	// Pitch (y-axis rotation)
	float y0 = 2.0f*(quat.w*quat.y - quat.z*quat.x);
	y0 = y0 > 1.0f ? 1.0f : y0;
	y0 = y0 < -1.0f ? -1.0f : y0;
	result.y = asinf(y0);

	// Yaw (z-axis rotation)
	float z0 = 2.0f*(quat.w*quat.z + quat.x*quat.y);
	float z1 = 1.0f - 2.0f*(quat.y*quat.y + quat.z*quat.z);
	result.z = atan2f(z0, z1);

	return result;
}

static void AxisAngleFromQuat(Quat q, Vec3* outAxis, float* outAngle) {
	if (fabs(q.w) > 1.0f)
	{
		// QuaternionNormalize(q);
		float length = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
		if (length == 0.0f) length = 1.0f;
		float ilength = 1.0f/length;

		q.w = q.x*ilength;
		q.y = q.y*ilength;
		q.z = q.z*ilength;
		q.w = q.w*ilength;
	}

	Vec3 resAxis = { 0.0f, 0.0f, 0.0f };
	float resAngle = 2.0f*acosf(q.w);
	float den = sqrtf(1.0f - q.w*q.w);

	if (den > 0.0001f)
	{
		resAxis.x = q.x/den;
		resAxis.y = q.y/den;
		resAxis.z = q.z/den;
	}
	else
	{
		// This occurs when the angle is zero.
		// Not a problem: just set an arbitrary normalized axis.
		resAxis.x = 1.0f;
	}

	*outAxis = resAxis;
	*outAngle = resAngle;
}

// from and to are direction vectors
static Quat QuatFromDirectionChange(Vec3 from, Vec3 to) {
	Quat result = {};
	float cos = V3Dot(from, to);
	if (Abs(cos - (-1.0f)) < 0.000001f) {
		result = QuatFromAxisAngle(V3(0.0f, 1.0f, 0.0f), DegreesToRadians(180.0f));
	}
	if (Abs(cos - (1.0f)) < 0.00001f) {
		result = QuatI();
	}
	float angle = acosf(cos);
	Vec3 axis = V3Cross(from, to);
	axis = V3Norm(axis);
	result = QuatFromAxisAngle(axis, angle);
	return result;
}

// Faster version from ryg 
// https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/

static Vec3 RotateVecByQuat(Vec3 vec, Quat quat) {
	Vec3 result = {};
	Vec3 a = V3Cross(V3MulF(quat.xyz, 2), vec);
	Vec3 b = V3Cross(quat.xyz, a);
	Vec3 c = V3MulF(a, quat.w);
	result = V3Add(V3Add(vec, c), b);
	return result;
}

//static Quat QuatLookAt(Vec3 position, Vec3 target) {
//}

// High Level API
static Vec3 GetForwardVector(Quat quat) {	return V3Norm(RotateVecByQuat(V3Forward(), quat)); } 
static Vec3 GetRightVector(Quat quat) {	return V3Norm(RotateVecByQuat(V3Right(), quat)); } 
static Vec3 GetUpVector(Quat quat) {	return V3Norm(RotateVecByQuat(V3Up(), quat)); } 

static Mat4 MakeTransformMatrix(Transform transform) {
	Mat4 result = M4I();
	Mat4 translation = M4Translate(transform.position);
	Mat4 rotation = M4FromQuat(transform.rotation);
	Mat4 scale = M4Scale(transform.scale);

	// Checked
	//result = M4Mul(M4Mul(translation, rotation), scale);
	result = M4Mul(M4Mul(scale, rotation), translation);

	return result;
}