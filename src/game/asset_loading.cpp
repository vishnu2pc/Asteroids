#include "asset_info.h"

struct GameAssets {
	MemoryArena* permanent_arena;
	u32 offsets[ASSET_BLOB_TOTAL];

	u8* data;
	u32 size;

	TextureAssetInfo* texture_assets;
	MeshAssetInfo* mesh_assets;
};

static void*
GetAssetBlob(ASSET_BLOB blob, GameAssets* assets) {
	return assets->data + assets->offsets[blob];
}

static GameAssets* 
LoadGameAssets(MemoryArena* arena) {
	GameAssets* ga = {};

	PlatformFileInfo info;
	u8 name[] = "data.gaf";
	info.name = name;
	PlatformFileHandle handle = platform_api.open_file(&info);
	Assert(!handle.failed);

	ga = PushStruct(arena, GameAssets);
	ga->permanent_arena = arena;

	u8* memblock = (u8*)PushSize(arena, info.size);
	ga->data = memblock; 
	ga->size = info.size;

	platform_api.read_file(&handle, info.size, ga->data);
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

	for(u8 i=0; i<VERTEX_BUFFER_TOTAL; i++) {
		if(StringCompare(vbf->type, vertex_buffer_names[i])) {
			vbd.type = (VERTEX_BUFFER)i;
			break;
		}
	}

	vbd.data = (float*)(ga->data + vbf->offset_to_data);

	return vbd;
}

static MeshFormat*
GetMeshFormat(char* name, GameAssets* ga) {
	MeshFormat* result = 0;

	MeshesBlob* mb = (MeshesBlob*)(ga->data + ga->offsets[ASSET_BLOB_MESHES]);
	MeshFormat* mf_arr = (MeshFormat*)(ga->data + mb->offset_to_mesh_formats);

	u32 i=0;
	while(!StringCompare(mf_arr[i].name, name)) i++;
	result = mf_arr + i;

	return result;
}

MeshData* 
LoadMeshData(MeshFormat* mf, GameAssets* ga) {
	MeshData* md = PushStruct(ga->permanent_arena, MeshData);

	md->vertices_count = mf->vertices_count;
	md->indices_count = mf->indices_count;
	md->vb_data_count = mf->vertex_buffer_count;
	md->indices = (u32*)(ga->data + mf->offset_to_indices);
	md->vb_data = PushArray(ga->permanent_arena, VertexBufferData, mf->vertex_buffer_count);

	VertexBufferFormat* vbf_arr = (VertexBufferFormat*)(ga->data + mf->offset_to_vertex_buffers);
	for(u8 i=0; i<mf->vertex_buffer_count; i++) 
		md->vb_data[i] = MakeVertexBufferData(vbf_arr + i, ga);

	return md;
}

static MeshFormat*
GetAllMeshFormats(GameAssets* ga, u32* count) {
	MeshesBlob* tb = (MeshesBlob*)(ga->data + ga->offsets[ASSET_BLOB_MESHES]);
	MeshFormat* mf_arr = (MeshFormat*)(ga->data + tb->offset_to_mesh_formats);

	*count = tb->meshes_count;
	return mf_arr;
}

static TextureFormat*
GetAllTextureFormats(GameAssets* ga, u32* count) {
	TexturesBlob* tb = (TexturesBlob*)(ga->data + ga->offsets[ASSET_BLOB_TEXTURES]);
	TextureFormat* tf_arr = (TextureFormat*)(ga->data + tb->offset_to_texture_formats);

	*count = tb->textures_count;
	return tf_arr;
}

static FontFormat*
GetAllFontFormats(GameAssets* ga, u32* count) {
	FontsBlob* fb = (FontsBlob*)(ga->data + ga->offsets[ASSET_BLOB_FONTS]);
	FontFormat* ff_arr = (FontFormat*)(ga->data + fb->offset_to_font_formats);

	*count = fb->fonts_count;
	return ff_arr;
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

static FontFormat*
GetFontFormat(char* name, GameAssets* ga) {
	FontFormat* ff = 0;

	u32 count = 0;
	FontFormat* ff_arr = GetAllFontFormats(ga, &count);

	bool found = false;
	u32 i=0;
	for(i=0; i<count; i++) {
		if(StringCompare(ff_arr[i].name, name)) {
			found = true;
			break;
		}
	}
	Assert(found);

	ff = ff_arr + i;

	return ff;
}

static void*
GetFont(char* name, GameAssets* ga) {
	FontFormat* ff = GetFontFormat(name, ga);
	return ga->data + ff->offset_to_data;
}

