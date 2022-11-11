enum ENTITIY_PROPERTY {
	ENTITY_PROPERTY_Mesh                  = 1<<0,
	ENTITY_PROPERTY_TexturedQuad          = 1<<1,
	ENTITY_PROPERTY_SpinInPlace           = 1<<2,
	ENTITY_PROPERTY_PlayerControlled      = 1<<3,
	ENTITY_PROPERTY_BoundingBox           = 1<<4,
	ENTITY_PROPERTY_Movable           		= 1<<5,
	ENTITY_PROPERTY_SimpleChase           = 1<<6,
	ENTITY_PROPERTY_HasHealth             = 1<<7,
	ENTITY_PROPERTY_DealsCollisionDamage  = 1<<8,
	ENTITY_PROPERTY_TakesCollisionDamage  = 1<<9,
	ENTITY_PROPERTY_LevelBoundary  				= 1<<10,
	ENTITY_PROPERTY_EntitySpawner  				= 1<<11,
	ENTITY_PROPERTY_SpawnsParticleSystem  = 1<<12,
	ENTITY_PROPERTY_HasParticleSystem     = 1<<13,
	ENTITY_PROPERTY_TOTAL
};
enum DEV_MODE {
	DEV_MODE_PAUSED = 0x1,
};

enum AXIS { AXIS_X, AXIS_Y, AXIS_Z };
enum TEAM { TEAM_NONE, TEAM_PLAYER, TEAM_ENEMY, TEAM_TOTAL };
enum RESPONSE { RESPONSE_NONE, RESPONSE_DEFENSIVE, RESPONSE_HOSTILE };
enum BOUNDING_BOX_ORIENTATION { BOUNDING_BOX_ORIENTATION_NORMAL, BOUNDING_BOX_ORIENTATION_INVERSE };
enum ENTITY_FAB { ENTITY_FAB_MINE, ENTITY_FAB_PARTICLE_SYSTEM, ENTITY_FAB_TOTAL };

struct BoundingBox {
	Vec3 min;
	Vec3 max;
	Vec3 size;
};

#define MAX_SPAWNS 20
struct SpawnerInfo {
	Transform transforms[MAX_SPAWNS];
	u8 entity_types[MAX_SPAWNS];
	float delta_times[MAX_SPAWNS];
	u8 spawn_count;

	u8 spawn_id;
	float current_delta;
};

#define MAX_PARTICLES 10
struct ParticleSystem {
	TextureBuffer* textures[MAX_PARTICLES];
	u32 texture_count;

	u32 particle_count;

	float life_time;

	Vec3 initial_size;
	Vec3 final_size;

	Vec3 current_positions[MAX_PARTICLES];
	Vec3 final_positions[MAX_PARTICLES];
	Vec3 spawn_position;
	float current_time;
	bool init;
};

struct SpawnInfo {
	u8 type;
	Transform transform;
	ParticleSystem particle_system;
	SpawnInfo* next;
};

struct Entity {
	Entity* next;
	Entity* prev;

	u64 properties;

	u8 team;
	u8 response;

	Transform transform;
	MeshPipeline mesh_pipeline;
	TexturedQuad textured_quad;

	u8 spin_axis;
	float spin_amount;

	BoundingBox bb_mesh_space; 
	BoundingBox bb_object_space; 
	u8 bb_orientation;

	Vec3 chase_target;
	float speed;
	float acceleration;

	i32 health;
	u32 attack_damage;
	u32 damage_reduction;

	SpawnerInfo spawner_info;

	ParticleSystem particle_system;
};

#define MAX_ENTITIES 200
struct EntityBlob {
	MemoryArena* permanent_arena;
	MemoryArena* frame_arena;

	Entity* entities;
	Entity* first_free;
	u32 entity_count;

};

