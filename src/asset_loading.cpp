#include "asset_formats.cpp"
#include "file_formats.cpp"

struct GameAssets {
	u8* data;
	u32 offsets[ASSET_BLOB_TOTAL];
};

static GameAssets LoadAssetFile() {
	GameAssets ga = {};
	HANDLE h = CreateFileA("data.gaf", GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	assert(h != INVALID_HANDLE_VALUE);

	u64 size = 0;
	LARGE_INTEGER li = {};
	assert(GetFileSizeEx(h, &li));
	size = li.QuadPart;

	u8* assets = PushMaster(u8, size);
	DWORD bytes_read = 0;
	assert(ReadFile(h, assets, size, &bytes_read, 0));

	GameAssetFile* gaf = (GameAssetFile*)assets;
	assert(strcmp(gaf->identification, "gaf"));

	Directory* dir = (Directory*)(assets + gaf->offset_to_blob_directories);
	for(u8 i=0; i<gaf->number_of_blobs; i++) 
		for(u8 j=0; j<ASSET_BLOB_TOTAL; j++) 
			if(strcmp(blob_names[j], dir[i].name_of_blob) == 0)
				ga.offsets[j] = dir[i].offset_to_blob;

	ga.data = assets;
	return ga;
}

VertexBufferData MakeVertexBufferData(VertexBufferFormat* vbf, GameAssets* ga) {
	VertexBufferData vbd = {};

	bool found = false;
	for(u8 i=0; i<VERTEX_BUFFER_TOTAL; i++) {
		if(strcmp(vbf->type, vertex_buffer_names[i]) == 0) {
			vbd.type = (VERTEX_BUFFER)i;
			found = true;
			break;
		}
	}
	assert(found);

	vbd.data = (float*)(ga->data + vbf->offset_to_data);

	return vbd;
}

MeshData MakeMeshData(MeshFormat* mf, GameAssets* ga) {
	MeshData md = {};

	md.vertices_count = mf->vertices_count;
	md.indices_count = mf->indices_count;
	md.vb_data_count = mf->vertex_buffer_count;
	md.indices = (u32*)(ga->data + mf->offset_to_indices);
	md.vb_data = PushMaster(VertexBufferData, mf->vertex_buffer_count);

	VertexBufferFormat* vbf_arr = (VertexBufferFormat*)(ga->data + mf->offset_to_vertex_buffers);
	for(u8 i=0; i<mf->vertex_buffer_count; i++) 
		md.vb_data[i] = MakeVertexBufferData(vbf_arr + i, ga);

	return md;
}

MeshData LoadMeshDataFromAssets(char* name, GameAssets* ga) {
	assert(name);
	MeshData md = {};

	MeshesBlob* mb = (MeshesBlob*)(ga->data + ga->offsets[ASSET_BLOB_MESHES]);
	MeshFormat* mf_arr = (MeshFormat*)(ga->data + mb->offset_to_mesh_formats);

	MeshFormat* mf = 0; 
	u32 i=0;
	while(strcmp(mf_arr[i].mesh_name, name) != 0) i++;
	mf = mf_arr + i;

	assert(strcmp(mf->mesh_name, name) == 0);

	md = MakeMeshData(mf, ga);

	return md;
}