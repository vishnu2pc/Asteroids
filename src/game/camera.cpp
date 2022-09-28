struct Camera {
	Vec3 position;
	Quat rotation;
	float fov;
	float near_clip;
	float far_clip;
	float aspect_ratio;
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
DefaultCamera(WindowDimensions wd, MemoryArena* arena) {
	Camera* cam = PushStruct(arena, Camera);
	cam->position = V3Z();
	cam->rotation = QuatI();
	cam->fov = 90.0f;
	cam->near_clip = 0.1f;
	cam->far_clip = 1000.0f;
	cam->aspect_ratio = (float)wd.width/(float)wd.height;
	return cam;
}

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
	Mat4 perspective = M4Perspective(camera->fov, camera->aspect_ratio, camera->near_clip, camera->far_clip);
	result = M4Mul(perspective, view);
	return result;
}

static void
FirstPersonCamera(Camera* camera, FPControlInfo* fpci, Input* input) {
	FirstPersonControl(&camera->position, &camera->rotation, true, fpci, input);
}

