struct Camera {
	Vec3 position;
	Quat rotation;
	float near_clip;
	float far_clip;
	bool orthographic;
	union {
		struct {
			float fov;
			float aspect_ratio;
		};
		struct {
			float left;
			float right;
			float bottom;
			float top;
		};
	};
};

struct FPControlInfo {
	float base_sens;
	float trans_sens;
	float rot_sens;

	bool block_yaw;
	bool block_pitch;
};

static FPControlInfo
DefaultFPControlInfo() {
	FPControlInfo result = {};
	result.base_sens = 1.0f;
	result.trans_sens = 1.0f;
	result.rot_sens = 1.0f;
	return result;
}

static Camera*
DefaultPerspectiveCamera(WindowDimensions wd, MemoryArena* arena) {
	Camera* cam = PushStruct(arena, Camera);
	cam->position = V3Z();
	cam->rotation = QuatI();
	cam->fov = 90.0f;
	cam->near_clip = 0.1f;
	cam->far_clip = 1000.0f;
	cam->aspect_ratio = (float)wd.width/(float)wd.height;
	cam->orthographic = false;
	return cam;
}

static Camera*
DefaultOrthoCamera(WindowDimensions wd, MemoryArena* arena) {
	Camera* cam = PushStruct(arena, Camera);
	cam->position = V3(0.0f, 0.0f, 2000.0f);
	cam->rotation = QuatI();
	cam->fov = 90.0f;
	cam->near_clip = 0.1f;
	cam->far_clip = 1000.0f;
	cam->aspect_ratio = (float)wd.width/(float)wd.height;
	cam->orthographic = true;
	cam->left = 1000.0f;
	cam->right = 1000.0f;
	cam->bottom = 1000.0f;
	cam->top = 1000.0f;
	return cam;
}

static void 
FirstPersonControl(Vec3* position, Quat* rotation, bool camera, FPControlInfo* ci, Input* input) {
	float base_sens = ci->base_sens;
	float trans_sens = ci->trans_sens;
	float rot_sens = ci->rot_sens;

	float tdel = base_sens * trans_sens * 0.00001f;
	float rdel = base_sens * rot_sens * 0.5f;

	bool f = input->buttons[WIN32_BUTTON_W].held;
	bool b = input->buttons[WIN32_BUTTON_S].held;
	bool l = input->buttons[WIN32_BUTTON_A].held;
	bool r = input->buttons[WIN32_BUTTON_D].held;
	bool u = input->buttons[WIN32_BUTTON_Q].held;
	bool d = input->buttons[WIN32_BUTTON_E].held;

	float y = input->axes[WIN32_AXIS_MOUSE_DEL].x * !ci->block_yaw;
	float p = input->axes[WIN32_AXIS_MOUSE_DEL].y * !ci->block_pitch;

	Vec3 pos = *position;
	Quat rot = *rotation;

	// We move in the negative of the forward vector because camera
	Vec3 fv = camera ? V3Neg(GetForwardVector(rot)) : GetForwardVector(rot);
	Vec3 fdisp = V3MulF(fv, tdel);

	Vec3 rv = GetRightVector(rot);
	Vec3 rdisp = V3MulF(rv, tdel);

	Vec3 uv = V3Up();
	Vec3 udisp = V3MulF(uv, tdel);

	pos = V3Add(pos, V3MulF(V3Add(fv, fdisp), f));
	pos = V3Sub(pos, V3MulF(V3Add(fv, fdisp), b));

	pos = V3Add(pos, V3MulF(V3Add(rv, rdisp), r));
	pos = V3Sub(pos, V3MulF(V3Add(rv, rdisp), l));

	pos = V3Add(pos, V3MulF(V3Add(uv, rdisp), u));
	pos = V3Sub(pos, V3MulF(V3Add(uv, rdisp), d));

	Quat yrot = y ? QuatFromAxisAngle(V3Up(), -y * rdel) : QuatI();
	Quat xrot = p ? QuatFromAxisAngle(rv, -p * rdel) : QuatI();

	rot = QuatMul(QuatMul(yrot, xrot), rot);

	*position = pos;
	*rotation = rot;
}

/*
static void 
CameraDrawDebugText(Camera* cam, DebugText* dt, MemoryArena* arena) {
	DrawDebugText("CAMERA INFO", YELLOW, QUADRANT_TOP_LEFT, dt, arena);
	{
		char* char_string = PushArray(arena, char, 100);
		stbsp_sprintf(char_string, "Position: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
		DrawDebugText(char_string, WHITE, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		Vec3 euler = EulerFromQuat(cam->rotation);
		stbsp_sprintf(char_string, "Rotation: %.2f, %.2f, %.2f", RadToDeg(euler.x), RadToDeg(euler.y), RadToDeg(euler.z));
		//Vec3 axis;
		//float angle;
		//AxisAngleFromQuat(cam->rotation, &axis, &angle);
		//stbs_sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", axis.x, axis.y, axis.z, angle);
		//stbs_sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", cam->rotation.x, cam->rotation.y, cam->rotation.z, cam->rotation.w);
		DrawDebugText(char_string, WHITE, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		stbsp_sprintf(char_string, "FOV: %.2f", cam->fov);
		DrawDebugText(char_string, WHITE, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		stbsp_sprintf(char_string, "Aspect Ration: %.2f", cam->aspect_ratio);
		DrawDebugText(char_string, WHITE, QUADRANT_TOP_LEFT, dt, arena);
	}
}
*/

static Mat4 
MakeViewPerspective(Camera* camera) {
	Mat4 result = M4I();
	Mat4 translation = M4Translate(V3Neg(camera->position));
	/*
		 Vec3 direction = V3(0.0f, 0.0f, -1.0f);
		 Vec3 rotated_dir = RotateVecByQuat(direction, camera.rotation);
		 Vec3 target_disp = V3MulF(rotated_dir, 2.0f);

		 Vec3 target = V3Add(target_disp, camera.position);
		 Mat4 view = M4LookAt(camera.position, target, V3Up());
		 */

	Mat4 rot = M4FromQuat(camera->rotation);
	Mat4 view = M4Mul(M4Transpose(rot), translation);
	if(!camera->orthographic) {
		Mat4 perspective = M4Perspective(camera->fov, camera->aspect_ratio, camera->near_clip, camera->far_clip);
		result = M4Mul(perspective, view);
	}
	else {
		Mat4 orthographic = M4Orthographic(camera->left, camera->right, camera->bottom, camera->top,
				camera->near_clip, camera->far_clip);
		result = M4Mul(orthographic, view);
	}
	return result;
}

static void
FirstPersonCamera(Camera* camera, FPControlInfo* fpci, Input* input) {
	FirstPersonControl(&camera->position, &camera->rotation, true, fpci, input);
}

