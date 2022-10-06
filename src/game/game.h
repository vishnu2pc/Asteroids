struct GameState {
	MemoryArena total_arena;
	MemoryArena* frame_arena;
	TemporaryMemory frame_arena_temp;

	GameAssets* assets;
	TextUI* text_ui;
	Renderer* renderer;
	QuadRenderer* quad_renderer;
	MeshRenderer* mesh_renderer;
	Camera* camera;

	Entity* entity;
};
