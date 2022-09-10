struct CameraInfo {
	Vec3 position;
	Quat rotation;
	float fov;
	float near_clip;
	float far_clip;
	float aspect_ratio;
};

void CameraDrawDebugText(CameraInfo* cam, DebugText* dt) {
	DrawDebugText("CAMERA INFO", BLUE, 1, QUADRANT_TOP_LEFT, dt);
	{
		char* char_string = PushScratch(char, 100);
		sprintf(char_string, "Position: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
		DrawDebugText(char_string, MAROON, 1, QUADRANT_TOP_LEFT, dt);
		PopScratch(char, 100);
	}

	{
		char* char_string = PushScratch(char, 100);
		Vec3 euler = EulerFromQuat(cam->rotation);
		sprintf(char_string, "Rotation: %.2f, %.2f, %.2f", RadToDeg(euler.x), RadToDeg(euler.y), RadToDeg(euler.z));
		//Vec3 axis;
		//float angle;
		//AxisAngleFromQuat(cam->rotation, &axis, &angle);
		//sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", axis.x, axis.y, axis.z, angle);
		//sprintf(char_string, "Rotation: %.2f, %.2f, %.2f, %.2f", cam->rotation.x, cam->rotation.y, cam->rotation.z, cam->rotation.w);
		DrawDebugText(char_string, MAROON, 1, QUADRANT_TOP_LEFT, dt);
		PopScratch(char, 100);
	}

	{
		char* char_string = PushScratch(char, 100);
		sprintf(char_string, "FOV: %.2f", cam->fov);
		DrawDebugText(char_string, MAROON, 1, QUADRANT_TOP_LEFT, dt);
		PopScratch(char, 100);
	}

	{
		char* char_string = PushScratch(char, 100);
		sprintf(char_string, "Aspect Ration: %.2f", cam->aspect_ratio);
		DrawDebugText(char_string, MAROON, 1, QUADRANT_TOP_LEFT, dt);
		PopScratch(char, 100);
	}


}