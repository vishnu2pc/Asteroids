enum VERTEX_BUFFER {	
	VERTEX_BUFFER_NOT_SET,
	VERTEX_BUFFER_POSITION, 
	VERTEX_BUFFER_NORMAL, 
	VERTEX_BUFFER_TANGENT,
	VERTEX_BUFFER_COLOR, 
	VERTEX_BUFFER_TEXCOORD, 
	VERTEX_BUFFER_TOTAL
};

struct VertexBufferData {
	void* data;
	VERTEX_BUFFER type;
};

struct MeshData {
	VertexBufferData* vb_data;
	u32 vb_data_count;
		
	u32* indices;
	u32 vertices_count;
	u32 indices_count;
};

enum TEXTURE_SLOT {
	TEXTURE_SLOT_DIFFUSE,
	TEXTURE_SLOT_NORMAL,
	TEXTURE_SLOT_TOTAL
};

struct TextureData {
	TEXTURE_SLOT type;
	void* pixels;
	u32 width;
	u32 height;
	u8 num_components;
};

struct MaterialData {
	TextureData* texture_data;
	u8 count;
	char* name;
};
