
enum ASSET_TYPE {
	ASSET_TYPE_MODEL,
	ASSET_TYPE_TOTAL
};

enum BLOB {
	BLOB_MESHES,
	BLOB_TOTAL
};

enum FORMAT {
	FORMAT_MESH,
	FORMAT_VERTEX_BUFFER,
	FORMAT_TOTAL
};

enum STRING_LENGTH {
	STRING_LENGTH_BLOB = 8,
	STRING_LENGTH_VERTEX_BUFFER = 8,
	STRING_LENGTH_MESH = 12,
};

char* asset_path_dir[ASSET_TYPE_TOTAL] = { 
                                         "../../assets/models"
};

char* asset_file_format[ASSET_TYPE_TOTAL] = {
                                            "gltf"
};

char* blob_names[BLOB_TOTAL] = {
                               "meshes"
};

struct GameAssetFileFormat {
	char identification[4];
	u8 number_of_blobs;
	u32 offset_to_blob_directories;
};

struct Directory {
	u32 offset_to_blob;
	u32 size_of_blob;
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
