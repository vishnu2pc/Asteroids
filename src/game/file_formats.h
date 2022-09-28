enum STRING_LENGTH {
	STRING_LENGTH_BLOB = 12,
	STRING_LENGTH_VERTEX_BUFFER = 12,
	STRING_LENGTH_MESH = 12,
	STRING_LENGTH_TEXTURE = 12,
	STRING_LENGTH_TEXTURE_TYPE = 12,
};

enum ASSET_BLOB {
	ASSET_BLOB_MESHES,
	ASSET_BLOB_TEXTURES,
	ASSET_BLOB_TOTAL
};

char* blob_names[ASSET_BLOB_TOTAL] = {
	"meshes",
	"textures"
};

char* vertex_buffer_names[VERTEX_BUFFER_TOTAL] = {
	"NOT SET",
	"POSITION",
	"NORMAL",
	"TANGENT",
	"COLOR",
	"TEXCOORD"
};

char* texture_type_names[TEXTURE_SLOT_TOTAL] = {
	"DIFFUSE",
	"NORMAL"
};

//TODO:Roll your own string lib
struct GameAssetFile {
	char identification[5];
	u8 number_of_blobs;
	u32 offset_to_blob_directories;
};

struct Directory {
	u32 offset_to_blob;
	char name_of_blob[STRING_LENGTH_BLOB];
};

struct MeshesBlob {
	u32 meshes_count;
	u32 offset_to_mesh_formats;
};

struct TexturesBlob {
	u32 textures_count;
	u32 offset_to_texture_formats;
};

struct VertexBufferFormat {
	char type[STRING_LENGTH_VERTEX_BUFFER];
	u32 offset_to_data;
};

struct MeshFormat {
	char name[STRING_LENGTH_MESH];
	u8 vertex_buffer_count;
	u32 vertices_count;
	u32 indices_count;
	u32 offset_to_indices;
	u32 offset_to_vertex_buffers;
};

struct TextureFormat {
	char name[STRING_LENGTH_TEXTURE];
	char type[STRING_LENGTH_TEXTURE_TYPE];
	u32 width;
	u32 height;
	u32 num_components;
	u32 offset_to_data;
};
















