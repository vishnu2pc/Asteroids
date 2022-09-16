struct CameraInfo {
	Vec3 position;
	Quat rotation;
	float fov;
	float near_clip;
	float far_clip;
	float aspect_ratio;
};

struct FPControlInfo {
	float base_sens;
	float trans_sens;
	float rot_sens;

	bool block_yaw;
	bool block_pitch;
};

void CameraDrawDebugText(CameraInfo* cam, DebugText* dt, MemoryArena* arena) {
	DrawDebugText("CAMERA INFO", BLUE, QUADRANT_TOP_LEFT, dt, arena);
	{
		char* char_string = PushArray(arena, char, 100);
		sprintf(char_string, "Position: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
		DrawDebugText(char_string, MAROON, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		Vec3 euler = EulerFromQuat(cam->rotation);
		sprintf(char_string, "Rotation: %.2f, %.2f, %.2f", RadToDeg(euler.x), RadToDeg(euler.y), RadToDeg(euler.z));
		//Vec3 axis;
		//float angle;
		//AxisAngleFromQuat(cam->rotation, &axis, &angle);
		//sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", axis.x, axis.y, axis.z, angle);
		//sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", cam->rotation.x, cam->rotation.y, cam->rotation.z, cam->rotation.w);
		DrawDebugText(char_string, MAROON, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		sprintf(char_string, "FOV: %.2f", cam->fov);
		DrawDebugText(char_string, MAROON, QUADRANT_TOP_LEFT, dt, arena);
	}
	{
		char* char_string = PushArray(arena, char, 100);
		sprintf(char_string, "Aspect Ration: %.2f", cam->aspect_ratio);
		DrawDebugText(char_string, MAROON, QUADRANT_TOP_LEFT, dt, arena);
	}
}

static Mat4 MakeViewPerspective(CameraInfo camera) {
	Mat4 result = M4I();
	Mat4 translation = M4Translate(V3Neg(camera.position));
	/*
		Vec3 direction = V3(0.0f, 0.0f, -1.0f);
		Vec3 rotated_dir = RotateVecByQuat(direction, camera.rotation);
		Vec3 target_disp = V3MulF(rotated_dir, 2.0f);

		Vec3 target = V3Add(target_disp, camera.position);
		Mat4 view = M4LookAt(camera.position, target, V3Up());
		*/

	Mat4 rot = M4FromQuat(camera.rotation);
	Mat4 view = M4Mul(M4Transpose(rot), translation);
	Mat4 perspective = M4Perspective(camera.fov, camera.aspect_ratio, camera.near_clip, camera.far_clip);
	result = M4Mul(perspective, view);
	return result;
}
