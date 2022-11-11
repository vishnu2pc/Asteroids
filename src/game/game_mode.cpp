static void 
InitTestMode(GameState* game_state, WindowDimensions dim) {

	Transform transform = TransformI();
	transform.position = V3(-400.0f, 0.0f, 0.0f);
	transform.rotation = QuatMul(QuatFromEuler(0.0f, 0.0f, DegToRad(180.0f)), transform.rotation);
	transform.scale = V3(0.01f, 0.01f, 0.01f);

	game_state->camera = DefaultPerspectiveCamera(dim, &game_state->total_arena);
	game_state->camera->position = V3(0.0f, 0.0f, 300.0f);

	SpawnPlayer(transform, game_state);
	BoundingBox bb = { V3(-600.0f, -600.0f, -50.0f), V3(600.0f, 600.0f, 50.0f) };
	SpawnLevelBoundary(bb, game_state);

	SpawnerInfo info = {};
	u32 multiple = 1;
	for(i32 i=-500; i<=500; i+=100) {
		info.transforms[info.spawn_count] = TransformI();
		info.transforms[info.spawn_count].position = V3( 500.0f, (float)i, 0.0f);
		info.entity_types[info.spawn_count] = ENTITY_FAB_MINE;
		info.delta_times[info.spawn_count] = 1000 - 50*multiple++;
		info.spawn_count++;
	}
	SpawnEntitySpawner(&info, game_state);

	SpawnMine(TransformI(), game_state);

	game_state->test_mode.init_done = true;
}

static void
UpdateTestMode(GameState* game_state, Input* input, WindowDimensions wd) {
	if(!game_state->test_mode.init_done) InitTestMode(game_state, wd);

	LightInfo light = { V3Z(), 0.5f };
	game_state->mesh_renderer->light = light;

	UpdateEntities(game_state, input);
}

