struct CameraInfo {
	Vec3 position;
	Quat rotation;
	float fov;
	float near_clip;
	float far_clip;
	float aspect_ratio;
};

struct FPControlInfo {
	bool free_rotation;
	Vec3 UpAxis;
	float base_sens;
	float trans_sens;
	float rot_sens;
};

static Mat4 MakeViewPerspective(CameraInfo camera) {
	Mat4 result = M4I();
	Mat4 translation = M4Translate(camera.position);
	Mat4 rotation = M4FromQuat(camera.rotation);

	Mat4 view = M4Mul(translation, rotation);
	Mat4 perspective = M4Perspective(camera.fov, camera.aspect_ratio, camera.near_clip, camera.far_clip);
	result = M4Mul(perspective, view);
	return result;
}

static Transform FirstPersonControl(Transform transform, FPControlInfo control_info) {
	Transform final = {};
	final = transform;

	float base_sens = control_info.base_sens;
	float trans_sens = control_info.trans_sens;
	float rot_sens = control_info.rot_sens;

	float tdel = base_sens * trans_sens * 0.01f;
	float rdel = base_sens * rot_sens * 0.01f;

	Vec3 pos = transform.position;
	Quat rot = transform.rotation;

	bool f = HINPUT.w.pressed;
	bool b = HINPUT.s.pressed;
	bool l = HINPUT.a.pressed;
	bool r = HINPUT.d.pressed;
	bool u = HINPUT.shift.pressed;
	bool d = HINPUT.ctrl.pressed;

	int p = HINPUT.my.del;
	int y = HINPUT.mx.del;

	Vec3 fv = GetForwardVector(rot);
	Vec3 fdisp = V3MulF(fv, tdel);

	Vec3 rv = GetRightVector(rot);
	Vec3 rdisp = V3MulF(rv, tdel);

	Vec3 uv = V3Up();
	Vec3 udisp = V3MulF(uv, tdel);

	final.position = V3Add(final.position, V3MulF(V3Add(fv, fdisp), f));
	final.position = V3Sub(final.position, V3MulF(V3Add(fv, fdisp), b));

	final.position = V3Add(final.position, V3MulF(V3Add(rv, rdisp), r));
	final.position = V3Sub(final.position, V3MulF(V3Add(rv, rdisp), l));

	final.position = V3Add(final.position, V3MulF(V3Add(uv, rdisp), u));
	final.position = V3Sub(final.position, V3MulF(V3Add(uv, rdisp), d));

	Quat yrot = p ? QuatFromAxisAngle(V3Up(), rdel) : QuatI();
	Quat xrot = y ? QuatFromAxisAngle(rv, rdel) : QuatI();

	final.rotation = QuatMul(QuatMul(yrot, xrot), rot);
	final.scale = transform.scale;

	return final;
}