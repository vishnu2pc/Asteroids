static Entity*
AllocEntity(EntityBlob* blob) {
	Entity* result = blob->first_free;

	if(result) {
		blob->first_free = blob->first_free->next;
		ZeroMem(result, sizeof(Entity));
	}
	else result = PushStruct(blob->permanent_arena, Entity);

	result->next = blob->entities;
	if(blob->entities) blob->entities->prev = result;
	blob->entities = result;

	blob->entity_count++;
	return result;
}

static void
ReleaseEntity(Entity* entity, EntityBlob* blob) {
	if(entity->prev && entity->next) {
		entity->prev->next = entity->next;
		entity->next->prev = entity->prev;
	}
	if(entity->prev && !entity->next) entity->prev->next = 0; 
	if(!entity->prev && entity->next) entity->next->prev = 0;

	blob->entity_count--;
	entity->next = blob->first_free;
	blob->first_free = entity;
}

static void
UpdateTransform(Transform* inout, Transform var) {
	inout->position = V3Add(var.position, inout->position);
	inout->rotation = QuatMul(var.rotation, inout->rotation);
	inout->scale = V3Mul(var.scale, inout->scale);
}

static BoundingBox
UpdateBoundingBox(BoundingBox* box, Transform* transform) {
	BoundingBox obb = {};
	BoundingBox result = {};

	obb.min = V3Mul(transform->scale, box->min);
	obb.max = V3Mul(transform->scale, box->max);

	Mat4 rot_matrix = M4FromQuat(transform->rotation);
	Vec4 min = M4MulV(rot_matrix, V4FromV3(obb.min, 1.0f));
	Vec4 max = M4MulV(rot_matrix, V4FromV3(obb.max, 1.0f));
	obb.min = V3(min.x, min.y, min.z);
	obb.max = V3(max.x, max.y, max.z);

	obb.min = V3Add(obb.min, transform->position);
	obb.max = V3Add(obb.max, transform->position);

	result.min.x = obb.min.x < obb.max.x ? obb.min.x : obb.max.x;
	result.min.y = obb.min.y < obb.max.y ? obb.min.y : obb.max.y;
	result.min.z = obb.min.z < obb.max.z ? obb.min.z : obb.max.z;

	result.max.x = obb.max.x > obb.min.x ? obb.max.x : obb.min.x;
	result.max.y = obb.max.y > obb.min.y ? obb.max.y : obb.min.y;
	result.max.z = obb.max.z > obb.min.z ? obb.max.z : obb.min.z;

	result.size.x = Abs((result.max.x - result.min.x)/2);
	result.size.y = Abs((result.max.y - result.min.y)/2);
	result.size.z = Abs((result.max.z - result.min.z)/2);

	return result;
}

static bool
BoundingBoxIntersect(BoundingBox* left, BoundingBox* right) {
	bool x1 = left->min.x <= right->max.x;
	bool x2 = left->max.x >= right->min.x;
	bool y1 = left->min.y <= right->max.y;
	bool y2 = left->max.y >= right->min.y;
	bool z1 = left->min.z <= right->max.z;
	bool z2 = left->max.z >= right->min.z;
	return x1 && x2 && y1 && y2 && z1 && z2;
}

static BoundingBox
GetBoundingBoxFromMeshData(MeshData* mesh_data) {
	BoundingBox result = {};

	u8 i=0;
	for(i=0; i<mesh_data->vb_data_count; i++) 
		if(mesh_data->vb_data[i].type == VERTEX_BUFFER_POSITION) break;

	Vec3 max = V3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	Vec3 min = V3( FLT_MAX,  FLT_MAX,  FLT_MAX);

	Vec3* data = (Vec3*)mesh_data->vb_data[i].data;
	for(u32 j=0; j<mesh_data->vertices_count; j++) {
		if(data[j].x < min.x) min.x = data[j].x;
		if(data[j].y < min.y) min.y = data[j].y;
		if(data[j].z < min.z) min.z = data[j].z;

		if(data[j].x > max.x) max.x = data[j].x;
		if(data[j].y > max.y) max.y = data[j].y;
		if(data[j].z > max.z) max.z = data[j].z;
	}

	result.min = min;
	result.max = max;

	result.size.x = (result.min.x + result.max.x)/2;
	result.size.y = (result.min.y + result.max.y)/2;
	result.size.z = (result.min.z + result.max.z)/2;

	return result;
};

static Entity*
SpawnParticleSystem(ParticleSystem* particle_system, GameState* game_state) {
	Entity* result = AllocEntity(&game_state->entity_blob);
	result->properties |= ENTITY_PROPERTY_HasParticleSystem;

	result->particle_system = *particle_system;
	return result;
}

static Entity*
SpawnEntitySpawner(SpawnerInfo* info, GameState* game_state) {
	Entity* result = AllocEntity(&game_state->entity_blob);

	result->properties |= ENTITY_PROPERTY_EntitySpawner;

	result->spawner_info = *info;

	return result;
}

static Entity*
SpawnMine(Transform transform, GameState* game_state) {
	Entity* result = AllocEntity(&game_state->entity_blob);

	ParticleSystem particles = {};
	particles.textures[0] = GetTextureAssetInfo("lines_1", game_state->assets)->buffer;
	particles.textures[1] = GetTextureAssetInfo("lines_2", game_state->assets)->buffer;
	particles.textures[2] = GetTextureAssetInfo("lines_3", game_state->assets)->buffer;
	particles.textures[3] = GetTextureAssetInfo("lines_4", game_state->assets)->buffer;
	particles.textures[4] = GetTextureAssetInfo("lines_5", game_state->assets)->buffer;
	particles.textures[5] = GetTextureAssetInfo("lines_6", game_state->assets)->buffer;
	particles.textures[6] = GetTextureAssetInfo("lines_7", game_state->assets)->buffer;
	particles.textures[7] = GetTextureAssetInfo("lines_8", game_state->assets)->buffer;
	particles.textures[8] = GetTextureAssetInfo("lines_9", game_state->assets)->buffer;
	particles.textures[9] = GetTextureAssetInfo("lines_10", game_state->assets)->buffer;

	particles.texture_count = 10;
	particles.particle_count = 10;
	particles.life_time = 1000.0f;
	particles.initial_size = V3(1.0f, 1.0f, 1.0f);
	particles.final_size = V3Z();

	MeshAssetInfo* mesh_asset = GetMeshAssetInfo("Star", game_state->assets);

	result->properties |= ENTITY_PROPERTY_Mesh;
	result->properties |= ENTITY_PROPERTY_SpinInPlace;
	result->properties |= ENTITY_PROPERTY_BoundingBox;
	result->properties |= ENTITY_PROPERTY_SimpleChase;
	result->properties |= ENTITY_PROPERTY_DealsCollisionDamage;
	result->properties |= ENTITY_PROPERTY_TakesCollisionDamage;
	result->properties |= ENTITY_PROPERTY_HasHealth;
	result->properties |= ENTITY_PROPERTY_SpawnsParticleSystem;

	result->team = TEAM_ENEMY;
	result->response = RESPONSE_HOSTILE;
	result->health = 10;
	result->attack_damage = 10;

	result->bb_mesh_space = GetBoundingBoxFromMeshData(mesh_asset->data);

	result->transform = TransformI();
	result->transform.scale = V3(20.0f, 20.0f, 20.0f);
	result->particle_system = particles;

	UpdateTransform(&result->transform, transform);
	result->bb_object_space = UpdateBoundingBox(&result->bb_mesh_space, &result->transform);

	result->mesh_pipeline.mesh = *mesh_asset->mesh;
	result->spin_axis = AXIS_Z;
	result->spin_amount = 1.0f;

	result->speed = 1.0f;
	result->acceleration = 1.0f;

	return result;
}

static Entity*
SpawnPlayer(Transform transform, GameState* game_state) {
	Entity* result = AllocEntity(&game_state->entity_blob);

	MeshAssetInfo* mesh_asset = GetMeshAssetInfo("lc_1", game_state->assets);

	result->properties |= ENTITY_PROPERTY_Mesh;
	result->properties |= ENTITY_PROPERTY_PlayerControlled;
	result->properties |= ENTITY_PROPERTY_BoundingBox;
	result->properties |= ENTITY_PROPERTY_HasHealth;
	result->properties |= ENTITY_PROPERTY_TakesCollisionDamage;
	result->properties |= ENTITY_PROPERTY_DealsCollisionDamage;
	result->properties |= ENTITY_PROPERTY_Movable;

	result->team = TEAM_PLAYER;
	result->response = RESPONSE_HOSTILE;
	result->health = 100.0f;
	result->attack_damage = 10.0f;

	result->bb_mesh_space = GetBoundingBoxFromMeshData(mesh_asset->data);
	result->transform = TransformI();
	result->transform.scale = V3(1.0f, 1.0f, 1.0f);

	UpdateTransform(&result->transform, transform);
	result->bb_object_space = UpdateBoundingBox(&result->bb_mesh_space, &result->transform);

	result->mesh_pipeline.mesh = *mesh_asset->mesh;

	return result;
}

static Entity*
SpawnLevelBoundary(BoundingBox box, GameState* game_state) {
	Entity* result = AllocEntity(&game_state->entity_blob);

	result->properties |= ENTITY_PROPERTY_BoundingBox;
	result->properties |= ENTITY_PROPERTY_LevelBoundary;

	result->team = TEAM_NONE;
	result->response = RESPONSE_NONE;

	result->transform = TransformI();
	result->bb_mesh_space = box;
	result->bb_object_space = box;
	result->bb_orientation = BOUNDING_BOX_ORIENTATION_INVERSE;

	return result;
}

static void
ResolveCollision(Entity* a, Entity* b) {
	if(a->properties & ENTITY_PROPERTY_LevelBoundary) {
		b->transform.position.x = Clamp(a->bb_object_space.min.x + b->bb_object_space.size.x, b->transform.position.x, a->bb_object_space.max.x - b->bb_object_space.size.x);
		b->transform.position.y = Clamp(a->bb_object_space.min.y + b->bb_object_space.size.y, b->transform.position.y, a->bb_object_space.max.y - b->bb_object_space.size.y);
	}
	if(b->properties & ENTITY_PROPERTY_LevelBoundary) {
		a->transform.position.x = Clamp(b->bb_object_space.min.x + a->bb_object_space.size.x, a->transform.position.x, b->bb_object_space.max.x - a->bb_object_space.size.x);
		a->transform.position.y = Clamp(b->bb_object_space.min.y + a->bb_object_space.size.y, a->transform.position.y, b->bb_object_space.max.y - a->bb_object_space.size.y);
	}
}

static void
ResolveDamageExchange(Entity* a, Entity* b) {
	if(a->properties & ENTITY_PROPERTY_DealsCollisionDamage &&
			b->properties & ENTITY_PROPERTY_TakesCollisionDamage) {
		b->health -= a->attack_damage - b->damage_reduction*a->attack_damage;
	}

	if(b->properties & ENTITY_PROPERTY_DealsCollisionDamage &&
			a->properties & ENTITY_PROPERTY_TakesCollisionDamage) {
		a->health -= b->attack_damage - a->damage_reduction*b->attack_damage;
	}
}

static void
UpdateEntities(GameState* game_state, Input* input) {
#define DEBUG_BOUNDING_BOX
	EntityBlob* blob = &game_state->entity_blob;
	Entity* entity = blob->entities;

	Entity** entities_with_bb = PushArrayClear(blob->frame_arena, Entity*, blob->entity_count);
	Entity** entities_to_release = PushArrayClear(blob->frame_arena, Entity*, blob->entity_count);

	SpawnInfo* entities_to_spawn = 0;
	u32 release_count = 0;
	u32 bb_count = 0;

	struct EntityTeam {
		Entity** entities;
		u32 count;
	};

	EntityTeam entities_on_team[TEAM_TOTAL] = {};
	for(u8 i=0; i<TEAM_TOTAL; i++) {
		entities_on_team[i].entities = PushArrayClear(blob->frame_arena, Entity*, blob->entity_count);
	}

	while(entity) {
		if(entity->properties & ENTITY_PROPERTY_BoundingBox) {
			entities_with_bb[bb_count++] = entity;
		}
		u8 team = entity->team;
		u32 count = entities_on_team[team].count;
		entities_on_team[team].entities[count] = entity;
		entities_on_team[team].count++;

		entity = entity->next;
	}

	for(u32 i=0; i<bb_count; i++) {
		for(u32 j=i+1; j<bb_count; j++) {
			Entity* a = entities_with_bb[i];
			Entity* b = entities_with_bb[j];

			if(BoundingBoxIntersect(&a->bb_object_space, &b->bb_object_space)) {
				if((a->team != b->team) && ((a->response == RESPONSE_HOSTILE) || (b->response == RESPONSE_HOSTILE)))
					ResolveDamageExchange(a, b);
				ResolveCollision(a, b);
			}
		}
	}

	entity = blob->entities;
	while(entity) {
		if(!game_state->dev_mode & DEV_MODE_PAUSED) {
			if(entity->properties & ENTITY_PROPERTY_SpinInPlace) {
				Quat spin;
				if(entity->spin_axis == AXIS_X)
					spin = QuatMulF(QuatFromEuler(0.1f, 0.0f, 0.0f), entity->spin_amount);
				if(entity->spin_axis == AXIS_Y)
					spin = QuatMulF(QuatFromEuler(0.0f, 0.1f, 0.0f), entity->spin_amount);
				if(entity->spin_axis == AXIS_Z)
					spin = QuatMulF(QuatFromEuler(0.0f, 0.0f, 0.1f), entity->spin_amount);

				entity->transform.rotation = QuatMul(spin, entity->transform.rotation); 
			}

			if(entity->properties & ENTITY_PROPERTY_SimpleChase) {
				Vec3 closest_target = V3Z();
				float closest_distance = FLT_MAX;

				for(u32 i=0; i<TEAM_TOTAL; i++) {
					if(entity->team == i) continue;

					for(u32 j=0; j<entities_on_team[i].count; j++) {
						Entity* other = entities_on_team[i].entities[j];

						if(other->team != entity->team && other->response == RESPONSE_HOSTILE) {
							float distance = V3Mag(V3Sub(entity->transform.position, other->transform.position));
							if(distance < closest_distance) {
								closest_distance = distance;
								closest_target = other->transform.position;
							}
						}

					}
				}

				Vec3 dir = V3Norm(V3Sub(closest_target, entity->transform.position));
				entity->transform.position = V3Add(entity->transform.position, 
						V3MulF(dir, entity->speed*entity->acceleration));
			}

			if(entity->properties & ENTITY_PROPERTY_PlayerControlled) {
				bool up = input->buttons[WIN32_BUTTON_W].held;
				bool down = input->buttons[WIN32_BUTTON_S].held;
				bool left = input->buttons[WIN32_BUTTON_A].held;
				bool right = input->buttons[WIN32_BUTTON_D].held;

				float scale_factor = V3Mag(entity->transform.scale) * 1000.0f;
				if(up) entity->transform.position.y += 0.2f * scale_factor;
				if(down) entity->transform.position.y -= 0.2f * scale_factor;
				if(left) entity->transform.position.x -= 0.2f * scale_factor;
				if(right) entity->transform.position.x += 0.2f * scale_factor;
			}
		}

		if(entity->properties & ENTITY_PROPERTY_EntitySpawner) {
			SpawnerInfo* info = &entity->spawner_info;
			if(info->spawn_id >= info->spawn_count) {
				entities_to_release[release_count++] = entity;
			}

			if(info->current_delta >= info->delta_times[info->spawn_id]) {

				SpawnInfo* last = entities_to_spawn;
				if(!entities_to_spawn) last = entities_to_spawn = PushStructClear(game_state->frame_arena, SpawnInfo);
				else {
					SpawnInfo* last_node = entities_to_spawn;
					while(last_node->next) last_node = last_node->next;

					last = last_node->next = PushStructClear(game_state->frame_arena, SpawnInfo);
				}
				last->type = info->entity_types[info->spawn_id];
				last->transform = info->transforms[info->spawn_id];
				info->current_delta = 0;
				info->spawn_id++;
			}
			else
			info->current_delta += game_state->timer.frame_time;
		}

		if(entity->properties & ENTITY_PROPERTY_Mesh) {
			entity->mesh_pipeline.info = PushStructClear(blob->frame_arena, MeshInfo);
			entity->mesh_pipeline.info->model = MakeTransformMatrix(entity->transform);
			entity->mesh_pipeline.info->color = V4FromV3(WHITE, 1.0f);
			PushMeshPipeline(entity->mesh_pipeline, game_state->mesh_renderer);
		}

		if(entity->properties & ENTITY_PROPERTY_TexturedQuad) {
			PushTexturedQuad(&entity->textured_quad.quad, 
					entity->textured_quad.texture, game_state->quad_renderer);
		}

		if(entity->properties & ENTITY_PROPERTY_HasHealth) {
			if(entity->health <= 0) {
				entities_to_release[release_count++] = entity;
#if 0 
				if(entity->properties & ENTITY_PROPERTY_SpawnsParticleSystem) {

					SpawnInfo* last = entities_to_spawn;
					if(!entities_to_spawn) last = entities_to_spawn = PushStructClear(game_state->frame_arena, SpawnInfo);
					else {
						SpawnInfo* last_node = entities_to_spawn;
						while(last_node->next) last_node = last_node->next;

						last = last_node->next = PushStructClear(game_state->frame_arena, SpawnInfo);
					}
					last->type = ENTITY_FAB_PARTICLE_SYSTEM;
					last->particle_system = entity->particle_system;
					last->particle_system.spawn_position = entity->transform.position;
				}
#endif
			}
		}
		
#if 0
		if(entity->properties & ENTITY_PROPERTY_HasParticleSystem) {
			ParticleSystem* particles = &entity->particle_system;
			if(particles->current_time >= particles->life_time) {
				entities_to_release[release_count++] = entity;
			}
			else {
				if(!particles->init) {
					particles->init = true;
					for(u8 i=0; i<particles->particle_count; i++) {
						Vec3 random_dir = V3Norm(V3(rand(), rand(), 0));
						float random_disp_1 = rand() % (50+1 + 50) - 50;
						float random_disp_2 = rand() % (50+1 + 50) - 50;
						Vec3 random_disp = V3(random_disp_1, random_disp_2, particles->spawn_position.z);
						particles->final_positions[i] = V3Add(V3Mul(random_dir, random_disp), 
								particles->spawn_position);
					}
				}
				else {
					for(u8 i=0; i<particles->particle_count; i++) {
						float t = particles->current_time;
						float d = particles->life_time;

						float bx = particles->spawn_position.x;
						float cx = particles->final_positions[i].x - bx;

						float by = particles->spawn_position.y;
						float cy = particles->final_positions[i].y - by;

						float bz = particles->spawn_position.z;
						float cz = particles->final_positions[i].z - bz;

						particles->current_positions[i].x = EaseSineOut(t, bx, cx, d);
						particles->current_positions[i].y = EaseSineOut(t, by, cy, d);
						particles->current_positions[i].z = EaseSineOut(t, bz, cz, d);

						Vec3 dir = V3Norm(V3Sub(particles->current_positions[i], particles->spawn_position));
						float length = 5.0f;
						float thickness = 5.0f;
						Line line = {};
						line.start = particles->current_positions[i];
						line.end = V3Add(line.start, V3MulF(dir, length));

						Vec3 line_unit_vector = V3Norm(V3Sub(line.end, line.start));
						Vec3 camera_forward = GetForwardVector(game_state->camera->rotation);
						Vec3 perp = V3Cross(camera_forward, line_unit_vector);

						Quad quad = MakeQuadFromLine(&line, thickness, perp);
						PushParticleQuad(&quad, particles->textures[i], game_state->quad_renderer);
					}
				}
				particles->current_time += game_state->timer.frame_time;
			}
		}
#endif

		if(entity->properties & ENTITY_PROPERTY_BoundingBox) {
			entity->bb_object_space = UpdateBoundingBox(&entity->bb_mesh_space, &entity->transform);
#ifdef DEBUG_BOUNDING_BOX
			Vec3 min = entity->bb_object_space.min;
			Vec3 max = entity->bb_object_space.max;
			Line l1 = { V3(min.x, min.y, min.z), V3(min.x, max.y, min.z) };
			Line l2 = { V3(min.x, min.y, max.z), V3(min.x, max.y, max.z) };
			Line l3 = { V3(max.x, min.y, min.z), V3(max.x, max.y, min.z) };
			Line l4 = { V3(max.x, min.y, max.z), V3(max.x, max.y, max.z) };
			Line l5 = { V3(min.x, min.y, min.z), V3(max.x, min.y, min.z) };
			Line l6 = { V3(min.x, min.y, max.z), V3(max.x, min.y, max.z) };
			Line l7 = { V3(min.x, max.y, min.z), V3(max.x, max.y, min.z) };
			Line l8 = { V3(min.x, max.y, max.z), V3(max.x, max.y, max.z) };
			Line l9 = { V3(min.x, min.y, min.z), V3(min.x, min.y, max.z) };
			Line l10 = { V3(min.x, max.y, min.z), V3(min.x, max.y, max.z) };
			Line l11 = { V3(max.x, min.y, min.z), V3(max.x, min.y, max.z) };
			Line l12 = { V3(max.x, max.y, min.z), V3(max.x, max.y, max.z) };
			PushRenderLine(&l1, V4FromV3(YELLOW, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l2, V4FromV3(BLUE, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l3, V4FromV3(RED, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l4, V4FromV3(GREEN, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l5, V4FromV3(MAGENTA, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l6, V4FromV3(PURPLE, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l7, V4FromV3(TEAL, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l8, V4FromV3(GREY, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l9, V4FromV3(WHITE, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l10, V4FromV3(MAROON, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l11, V4FromV3(OLIVE, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
			PushRenderLine(&l12, V4FromV3(CYAN, 1.0f), 2.0f, game_state->camera, game_state->quad_renderer);
#endif
		}
		entity = entity->next;
	}

	for(u32 i=0; i<release_count; i++) ReleaseEntity(entities_to_release[i], blob);
	SpawnInfo* last = entities_to_spawn;
	while(last) {
		if(last->type == ENTITY_FAB_MINE) 
			SpawnMine(last->transform, game_state);
		last = last->next;
	}
}













