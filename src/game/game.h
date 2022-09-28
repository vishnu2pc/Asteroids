struct Game {
	MemoryArena permanent;
	MemoryArena transient;

	GameAssets* assets;
	AssetInfo* asset_info;
	Renderer* renderer;
	QuadRenderer* quad_renderer;
	MeshRenderer* mesh_renderer;
	DebugText* debug_text;
	Camera* camera;

	Tetra* tetra;
};
