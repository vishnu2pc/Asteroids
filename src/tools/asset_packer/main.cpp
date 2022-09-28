#include <stdint.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CGLTF_IMPLEMENTATION
#include "../include/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include <intrin.h>
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

#include "buffers.cpp"
#include "../../game/asset_formats.h"
#include "../../game/file_formats.h"

enum ASSET_TYPE {
	ASSET_TYPE_MODEL,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_TOTAL
};

char* asset_path_dir[ASSET_TYPE_TOTAL] = { 
	"../assets/models",
	"../assets/textures"
};

char* asset_file_format[ASSET_TYPE_TOTAL] = {
	"gltf",
	"png"
};

enum FORMAT {
	FORMAT_GAME_ASSET_FILE,
	FORMAT_DIRECTORY,
	FORMAT_MESHES_BLOB,
	FORMAT_TEXTURES_BLOB,
	FORMAT_MESH,
	FORMAT_VERTEX_BUFFER,
	FORMAT_VERTEX_BUFFER_FLOAT,
	FORMAT_INDEX_BUFFER_U32,
	FORMAT_TEXTURE,
	FORMAT_PIXELS,

	FORMAT_TOTAL
};

u32 format_elem_sizes[FORMAT_TOTAL] {
	sizeof(GameAssetFile),
		sizeof(Directory),
		sizeof(MeshesBlob),
		sizeof(TexturesBlob),
		sizeof(MeshFormat),
		sizeof(VertexBufferFormat),
		sizeof(float),
		sizeof(u32),
		sizeof(TextureFormat),
		sizeof(u8)
};

char* StrPrepend(char* string, char* prepend) {
	u8 string_len = strlen(string);
	u8 prepend_len = strlen(prepend);
	char* result = (char*)calloc(string_len+prepend_len+1, sizeof(char));
	strcat(result, prepend);
	strcat(result+prepend_len, string);
	return result;
}

bool StrHasStrEnd(char* left, char* right) {
	u8 left_n = strlen(left);
	u8 right_n = strlen(right);

	if(left_n <= right_n) return false;
	u8 i = left_n-right_n;
	return strcmp(left+i, right) == 0;
}

char* MakeFullPath(char* dir, char* name, char* format) {
	char* result = 0;

	u8 dir_n = strlen(dir);
	u8 name_n = strlen(name);
	u8 format_n = strlen(format);

	u8 path_n = dir_n + name_n + format_n + 1;
	result = (char*)calloc(path_n, sizeof(char));
	strcpy(result, dir);
	strcpy(result+dir_n, "/");
	strcpy(result+dir_n+1, name);
	strcpy(result+dir_n+1+name_n, ".");
	strcpy(result+dir_n+1+name_n+1, format);

	return result;
}

struct FileInfo {
	char* name;  // filename without format
	char* format; // format prepended with .
	u64 size;
};

struct FolderInfo {
	char* dir;

	FileInfo* files;
	u32 file_count;
};

static u32 FindNumberOfFilesInDir(char* full_folder_dir, char* dot_file_format) {
	u32 file_count = 0;
	HANDLE h;
	WIN32_FIND_DATA fd;

	h = FindFirstFile(full_folder_dir, &fd);
	Assert(h != INVALID_HANDLE_VALUE);

	do {
		if(StrHasStrEnd(fd.cFileName, dot_file_format)) file_count++;
	} while(FindNextFile(h, &fd) != 0); 

	return file_count;
};

static FolderInfo LoadFolder(char* dir, char* file_format) {
	FolderInfo folder_info = {};

	char* dot_file_format;
	Assert(file_format[0] != '.');
	dot_file_format = StrPrepend(file_format, ".");

	char* full_folder_dir = (char*)calloc(strlen(dir)+3, sizeof(u8));
	strcpy(full_folder_dir, dir);
	strcat(full_folder_dir, "\\*");

	u32 file_count = FindNumberOfFilesInDir(full_folder_dir, dot_file_format);

	WIN32_FIND_DATA fd;
	HANDLE h;

	h = FindFirstFile(full_folder_dir, &fd);
	Assert(h != INVALID_HANDLE_VALUE);

	FileInfo* file_info_a = (FileInfo*)calloc(file_count, sizeof(FileInfo));
	u32 file_counter = 0;
	do {
		if(StrHasStrEnd(fd.cFileName, dot_file_format)) {
			FileInfo* fi = file_info_a + file_counter++;

			ULARGE_INTEGER ul;
			ul.HighPart = fd.nFileSizeHigh;
			ul.LowPart = fd.nFileSizeLow;
			ULONGLONG file_size = ul.QuadPart;

			char* ptr = strchr(fd.cFileName, '.');
			u8 name_len = ptr - fd.cFileName;
			char* file_name = (char*)calloc(name_len+1, sizeof(char));
			strncpy(file_name, fd.cFileName, name_len);

			fi->name = file_name;
			fi->format = file_format;
			fi->size = (u64)file_size;
		}
	} while(FindNextFile(h, &fd) != 0); 

	folder_info.dir = dir;
	folder_info.files = file_info_a;
	folder_info.file_count = file_count;

	return folder_info;
};

int main(int argc, char** argv) {

	StructBuffer struct_buffer[FORMAT_TOTAL] = {};
	u64 blob_offsets[ASSET_BLOB_TOTAL] = {};
	for(u8 i=0; i<FORMAT_TOTAL; i++) struct_buffer[i] = MakeStructBuffer(format_elem_sizes[i]);

	{
		StructBuffer* gaff_buffer = &struct_buffer[FORMAT_GAME_ASSET_FILE];
		GameAssetFile gaff = {};
		strcpy(gaff.identification, "gaff");
		gaff.number_of_blobs = ASSET_BLOB_TOTAL;
		PushStructBuffer(&gaff, 1, gaff_buffer);
	}
	{
		StructBuffer* ab_directory = &struct_buffer[FORMAT_DIRECTORY];
		for(u8 i=0; i<ASSET_BLOB_TOTAL; i++) {
			Directory dir = {};
			strcpy(dir.name_of_blob, blob_names[i]);
			PushStructBuffer(&dir, 1, ab_directory);
		}
	}
	{
		StructBuffer* ab_textures_blob = &struct_buffer[FORMAT_TEXTURES_BLOB];
		StructBuffer* ab_texture = &struct_buffer[FORMAT_TEXTURE];
		StructBuffer* ab_pixels = &struct_buffer[FORMAT_PIXELS];

		TexturesBlob textures_blob = {};

		char* dir = asset_path_dir[ASSET_TYPE_TEXTURE];
		char* file_format = asset_file_format[ASSET_TYPE_TEXTURE];
		FolderInfo folder = LoadFolder(dir, file_format);

		for(u8 i=0; i<folder.file_count; i++) {
			FileInfo file = folder.files[i];
			char* path = MakeFullPath(folder.dir, file.name, file.format);
			int x, y, n;
			void* data = stbi_load(path, &x, &y, &n, 4);
			Assert(data);

			TextureFormat texture_format = {};
			texture_format.offset_to_data = GetOffsetStructBuffer(ab_pixels);
			ReserveMemoryStructBuffer(x*y*4, ab_pixels);
			PushStructBuffer(data, x*y*4, ab_pixels);

			strcpy(texture_format.name, file.name);
			texture_format.name[STRING_LENGTH_TEXTURE - 1] = 0;
			strcpy(texture_format.type, texture_type_names[TEXTURE_SLOT_DIFFUSE]);
			texture_format.width = x;
			texture_format.height = y;
			texture_format.num_components = 4;

			PushStructBuffer(&texture_format, 1, ab_texture);
			textures_blob.textures_count++;
		}
		PushStructBuffer(&textures_blob, 1,ab_textures_blob);
	}
	{	// Loading Models
		StructBuffer* ab_meshes_blob = &struct_buffer[FORMAT_MESHES_BLOB];
		StructBuffer* ab_mesh = &struct_buffer[FORMAT_MESH];
		StructBuffer* ab_vertex_buffer = &struct_buffer[FORMAT_VERTEX_BUFFER];
		StructBuffer* ab_vertex_buffer_float = &struct_buffer[FORMAT_VERTEX_BUFFER_FLOAT];
		StructBuffer* ab_index_buffer_u32 = &struct_buffer[FORMAT_INDEX_BUFFER_U32];

		MeshesBlob meshes_blob = {};

		char* dir = asset_path_dir[ASSET_TYPE_MODEL];
		char* file_format = asset_file_format[ASSET_TYPE_MODEL];
		FolderInfo folder = LoadFolder(dir, file_format);

		// Loading cgltf data
		for(u8 i=0; i<folder.file_count; i++) {
			FileInfo file = folder.files[i];
			char* path = MakeFullPath(folder.dir, file.name, file.format);

			cgltf_data* data;
			cgltf_options opt = {};
			Assert(cgltf_parse_file(&opt, path, &data) == cgltf_result_success);
			Assert(cgltf_validate(data) == cgltf_result_success);
			Assert(cgltf_load_buffers(&opt, data, path) == cgltf_result_success);

			meshes_blob.meshes_count += data->meshes_count;
			for(u8 j=0; j<data->meshes_count; j++) {
				MeshFormat mesh_format = {};
				cgltf_mesh* mesh = data->meshes + j;
				Assert(mesh->name);
				strcpy(mesh_format.name, mesh->name);

				Assert(data->meshes[j].primitives_count == 1);
				cgltf_primitive* prim = mesh->primitives;
				Assert(prim->type == cgltf_primitive_type_triangles);

				Assert(prim->indices);
				mesh_format.indices_count = prim->indices->count;
				mesh_format.offset_to_indices = GetOffsetStructBuffer(ab_index_buffer_u32);
				mesh_format.offset_to_vertex_buffers = GetOffsetStructBuffer(ab_vertex_buffer);

				ReserveMemoryStructBuffer(prim->indices->count, ab_index_buffer_u32);
				for(u32 k=0; k<prim->indices->count; k++) {
					u32 index = (u32)cgltf_accessor_read_index(prim->indices, k);
					PushStructBuffer(&index, 1, ab_index_buffer_u32);
				}

				for(u8 k=0; k<prim->attributes_count; k++) {
					cgltf_attribute* att = prim->attributes + k;
					cgltf_accessor* acc = att->data;
					mesh_format.vertices_count = acc->count;

					cgltf_size float_count = cgltf_accessor_unpack_floats(acc, NULL, 0);
					switch(att->type) {
						case cgltf_attribute_type_position: {
							Assert(acc->component_type == cgltf_component_type_r_32f);
							Assert(acc->type == cgltf_type_vec3);

							mesh_format.vertex_buffer_count++;
							VertexBufferFormat vbf = {};
							strcpy(vbf.type, vertex_buffer_names[VERTEX_BUFFER_POSITION]);
							vbf.offset_to_data = GetOffsetStructBuffer(ab_vertex_buffer_float);

							ReserveMemoryStructBuffer(float_count, ab_vertex_buffer_float);
							cgltf_accessor_unpack_floats(acc, (float*)GetCursorStructBuffer(ab_vertex_buffer_float), float_count);
							ab_vertex_buffer_float->filled_count += float_count;

							PushStructBuffer(&vbf, 1, ab_vertex_buffer);
						} break;
						case cgltf_attribute_type_normal: {
							Assert(acc->component_type == cgltf_component_type_r_32f);
							Assert(acc->type == cgltf_type_vec3);

							mesh_format.vertex_buffer_count++;
							VertexBufferFormat vbf = {};
							strcpy(vbf.type, vertex_buffer_names[VERTEX_BUFFER_NORMAL]);
							vbf.offset_to_data = GetOffsetStructBuffer(ab_vertex_buffer_float);

							ReserveMemoryStructBuffer(float_count, ab_vertex_buffer_float);
							cgltf_accessor_unpack_floats(acc, (float*)GetCursorStructBuffer(ab_vertex_buffer_float), float_count);
							ab_vertex_buffer_float->filled_count += float_count;

							PushStructBuffer(&vbf, 1, ab_vertex_buffer);
						} break;
						case cgltf_attribute_type_texcoord: {
							Assert(acc->component_type == cgltf_component_type_r_32f);
							Assert(acc->type == cgltf_type_vec2);

							mesh_format.vertex_buffer_count++;
							VertexBufferFormat vbf = {};
							strcpy(vbf.type, vertex_buffer_names[VERTEX_BUFFER_TEXCOORD]);
							vbf.offset_to_data = GetOffsetStructBuffer(ab_vertex_buffer_float);

							ReserveMemoryStructBuffer(float_count, ab_vertex_buffer_float);
							cgltf_accessor_unpack_floats(acc, (float*)GetCursorStructBuffer(ab_vertex_buffer_float), float_count);
							ab_vertex_buffer_float->filled_count += float_count;

							PushStructBuffer(&vbf, 1, ab_vertex_buffer);
						} break;
					}
				}
				PushStructBuffer(&mesh_format, 1, ab_mesh);
			}
		}
		PushStructBuffer(&meshes_blob, 1, ab_meshes_blob);
		blob_offsets[ASSET_BLOB_MESHES] = GetOffsetStructBuffer(ab_meshes_blob) +
																			GetOffsetStructBuffer(ab_mesh) +
																			GetOffsetStructBuffer(ab_vertex_buffer) +
																			GetOffsetStructBuffer(ab_vertex_buffer_float) +
																			GetOffsetStructBuffer(ab_index_buffer_u32);
	}
	//------------------------------------------------------------------------
	GenericBuffer gb_file = {};
	{
		// Stitching up all buffers
		u32 offset = 0;
		for(u8 i=0; i<FORMAT_TOTAL; i++) gb_file.total_size += struct_buffer[i].elem_size * struct_buffer[i].filled_count;
		gb_file.data = calloc(gb_file.total_size, sizeof(u8));

		offset = GetOffsetStructBuffer(&struct_buffer[FORMAT_GAME_ASSET_FILE]);
		for(u32 i=0; i<struct_buffer[FORMAT_GAME_ASSET_FILE].filled_count; i++) {
			GameAssetFile* gaf = (GameAssetFile*)GetElementStructBuffer(&struct_buffer[FORMAT_GAME_ASSET_FILE], i);
			gaf->offset_to_blob_directories += offset;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_GAME_ASSET_FILE]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_DIRECTORY]);
		u32 blob_offset = 0;
		for(u32 i=0; i<struct_buffer[FORMAT_DIRECTORY].filled_count; i++) {
			Directory* dir = (Directory*)GetElementStructBuffer(&struct_buffer[FORMAT_DIRECTORY], i);
			dir->offset_to_blob += offset + blob_offset;
			blob_offset += blob_offsets[i];
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_DIRECTORY]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_MESHES_BLOB]);
		for(u32 i=0; i<struct_buffer[FORMAT_MESHES_BLOB].filled_count; i++) {
			MeshesBlob* mb = (MeshesBlob*)GetElementStructBuffer(&struct_buffer[FORMAT_MESHES_BLOB], i);
			mb->offset_to_mesh_formats += offset;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_MESHES_BLOB]);


		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_MESH]);
		for(u32 i=0; i<struct_buffer[FORMAT_MESH].filled_count; i++) {
			MeshFormat* mf = (MeshFormat*)GetElementStructBuffer(&struct_buffer[FORMAT_MESH], i);
			mf->offset_to_vertex_buffers += offset;
			u32 offset_vb = GetOffsetStructBuffer(&struct_buffer[FORMAT_VERTEX_BUFFER]);
			u32 offset_vb_float = GetOffsetStructBuffer(&struct_buffer[FORMAT_VERTEX_BUFFER_FLOAT]);
			mf->offset_to_indices += offset + offset_vb + offset_vb_float;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_MESH]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_VERTEX_BUFFER]);
		for(u32 i=0; i<struct_buffer[FORMAT_VERTEX_BUFFER].filled_count; i++) {
			VertexBufferFormat* vbf = (VertexBufferFormat*)GetElementStructBuffer(&struct_buffer[FORMAT_VERTEX_BUFFER], i);
			vbf->offset_to_data += offset;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_VERTEX_BUFFER]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_VERTEX_BUFFER_FLOAT]);
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_VERTEX_BUFFER_FLOAT]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_INDEX_BUFFER_U32]);
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_INDEX_BUFFER_U32]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_TEXTURES_BLOB]);
		for(u32 i=0; i<struct_buffer[FORMAT_TEXTURES_BLOB].filled_count; i++) {
			TexturesBlob* mb = (TexturesBlob*)GetElementStructBuffer(&struct_buffer[FORMAT_TEXTURES_BLOB], i);
			mb->offset_to_texture_formats += offset;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_TEXTURES_BLOB]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_TEXTURE]);
		for(u32 i=0; i<struct_buffer[FORMAT_TEXTURE].filled_count; i++) {
			TextureFormat* tf = (TextureFormat*)GetElementStructBuffer(&struct_buffer[FORMAT_TEXTURE], i);
			tf->offset_to_data += offset;
		}
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_TEXTURE]);

		offset += GetOffsetStructBuffer(&struct_buffer[FORMAT_PIXELS]);
		WriteStructBufferToGenericBuffer(&gb_file, &struct_buffer[FORMAT_PIXELS]);

	}
	//------------------------------------------------------------------------
	{ // Writing to File
		HANDLE h = CreateFileA("data.gaf", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		Assert(h != INVALID_HANDLE_VALUE);

		DWORD bytes_written = 0;
		Assert(WriteFile(h, gb_file.data, gb_file.filled_size, &bytes_written, 0));
		Assert(bytes_written == gb_file.filled_size);

	}
	return 1;
}
