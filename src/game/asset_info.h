struct TextureAssetInfo {
	TextureData* data;
	TextureBuffer* buffer;
	char* name;

	TextureAssetInfo* next;
};

struct MeshAssetInfo {
	MeshData* data;
	Mesh* mesh;
	char* name;

	MeshAssetInfo* next;
};
