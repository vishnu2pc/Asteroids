struct GameState {
	MemoryArena total_arena;
	MemoryArena* frame_arena;
	TemporaryMemory frame_arena_temp;

	Timer timer;

	GameAssets* assets;
	TextUI* text_ui;
	Renderer* renderer;
	QuadRenderer* quad_renderer;
	MeshRenderer* mesh_renderer;
	UIRenderer* ui_renderer;
	PostProcessRenderer* post_process_renderer;
	Camera* camera;

	EntityBlob entity_blob;

	GAME_MODE game_mode;
	TestMode test_mode;

	DEV_MODE dev_mode;

};
