#include "asset_formats.h"
#include "file_formats.h"

#define MAX_TEXTURES 255

struct GameAssets {
	MemoryArena permanent;
	u32 offsets[ASSET_BLOB_TOTAL];
	u8* data;
};

static void*
GetAssetBlob(ASSET_BLOB blob, GameAssets* assets) {
	return assets->data + assets->offsets[blob];
}

static GameAssets* 
InitGameAssets(PlatformAPI* platform_api, MemoryArena* arena) {
	GameAssets* ga = {};

	PlatformFileInfo info;
	u8 name[] = "data.gaf";
	info.name = name;
	PlatformFileHandle handle = platform_api->open_file(&info);
	Assert(!handle.failed);

	u8* memblock = GetMemory(arena, info.size + sizeof(GameAssets));
	ga = (GameAssets*)memblock;
	ga->permanent.memory.bp = memblock;
	ga->permanent.memory.size = info.size+sizeof(GameAssets);
	ga->permanent.used = info.size+sizeof(GameAssets);

	ga->data = memblock + sizeof(GameAssets);

	platform_api->read_file(&handle, info.size, ga->data);
	GameAssetFile* gaf = (GameAssetFile*)ga->data;
	Assert(StringCompare(gaf->identification, "gaff"));

	Directory* dir = (Directory*)(ga->data + gaf->offset_to_blob_directories);
	for(u8 i=0; i<gaf->number_of_blobs; i++) 
		for(u8 j=0; j<ASSET_BLOB_TOTAL; j++) 
			if(StringCompare(blob_names[j], dir[i].name_of_blob))
				ga->offsets[j] = dir[i].offset_to_blob;

	return ga;
}

VertexBufferData 
MakeVertexBufferData(VertexBufferFormat* vbf, GameAssets* ga) {
	VertexBufferData vbd = {};

	bool found = false;
	for(u8 i=0; i<VERTEX_BUFFER_TOTAL; i++) {
		if(StringCompare(vbf->type, vertex_buffer_names[i])) {
			vbd.type = (VERTEX_BUFFER)i;
			found = true;
			break;
		}
	}
	Assert(found);

	vbd.data = (float*)(ga->data + vbf->offset_to_data);

	return vbd;
}

MeshData 
MakeMeshData(MeshFormat* mf, GameAssets* ga, MemoryArena* arena) {
	MeshData md = {};

	md.vertices_count = mf->vertices_count;
	md.indices_count = mf->indices_count;
	md.vb_data_count = mf->vertex_buffer_count;
	md.indices = (u32*)(ga->data + mf->offset_to_indices);
	md.vb_data = PushArray(arena, VertexBufferData, mf->vertex_buffer_count);

	VertexBufferFormat* vbf_arr = (VertexBufferFormat*)(ga->data + mf->offset_to_vertex_buffers);
	for(u8 i=0; i<mf->vertex_buffer_count; i++) 
		md.vb_data[i] = MakeVertexBufferData(vbf_arr + i, ga);

	return md;
}

MeshData 
LoadMeshDataFromAssets(char* name, GameAssets* ga, MemoryArena* arena) {
	Assert(name);
	MeshData md = {};

	MeshesBlob* mb = (MeshesBlob*)(ga->data + ga->offsets[ASSET_BLOB_MESHES]);
	MeshFormat* mf_arr = (MeshFormat*)(ga->data + mb->offset_to_mesh_formats);

	MeshFormat* mf = 0; 
	u32 i=0;
	while(!StringCompare(mf_arr[i].name, name)) i++;
	mf = mf_arr + i;

	Assert(StringCompare(mf->name, name));

	md = MakeMeshData(mf, ga, arena);

	return md;
}
static TextureFormat*
GetAllTextureFormats(GameAssets* ga, u32* count) {
	TexturesBlob* tb = (TexturesBlob*)(ga->data + ga->offsets[ASSET_BLOB_TEXTURES]);
	TextureFormat* tf_arr = (TextureFormat*)(ga->data + tb->offset_to_texture_formats);

	*count = tb->textures_count;
	return tf_arr;
}

static TextureFormat*
GetTextureFormat(char* name, GameAssets* ga) {
	Assert(name);
	TextureFormat* tf = 0;

	u32 count = 0;
	TextureFormat* tf_arr = GetAllTextureFormats(ga, &count);

	bool found = false;
	u32 i=0;
	for(i=0; i<count; i++) {
		if(StringCompare(tf_arr[i].name, name)) {
			found = true;
			break;
		}
	}
	Assert(found);

	tf = tf_arr + i;
	return tf;
}

