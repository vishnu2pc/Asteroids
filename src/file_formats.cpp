enum STRING_LENGTH {
	STRING_LENGTH_BLOB = 8,
	STRING_LENGTH_VERTEX_BUFFER = 12,
	STRING_LENGTH_MESH = 12,
};

enum ASSET_BLOB {
	ASSET_BLOB_MESHES,
	ASSET_BLOB_TOTAL
};

char* blob_names[ASSET_BLOB_TOTAL] = {
	"meshes"
};

char* vertex_buffer_names[VERTEX_BUFFER_TOTAL] = {
	"NOT SET",
	"POSITION",
	"NORMAL",
	"TANGENT",
	"COLOR",
	"TEXCOORD"
};

struct GameAssetFile {
	char identification[4];
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

struct VertexBufferFormat {
	char type[STRING_LENGTH_VERTEX_BUFFER];
	u32 offset_to_data;
};

struct MeshFormat {
	char mesh_name[STRING_LENGTH_MESH];
	u8 vertex_buffer_count;
	u32 vertices_count;
	u32 indices_count;
	u32 offset_to_indices;
	u32 offset_to_vertex_buffers;
};
















