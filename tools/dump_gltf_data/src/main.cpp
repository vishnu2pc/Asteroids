#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include <intrin.h>
#define assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

#include "buffers.cpp"
#include "asset_formats.cpp"


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
	assert(h != INVALID_HANDLE_VALUE);

	do {
		if(StrHasStrEnd(fd.cFileName, dot_file_format)) file_count++;
	} while(FindNextFile(h, &fd) != 0); 

	return file_count;
};

static FolderInfo LoadFolder(char* dir, char* file_format) {
	FolderInfo folder_info = {};

	char* dot_file_format;
	assert(file_format[0] != '.');
	dot_file_format = StrPrepend(file_format, ".");

	char* full_folder_dir = (char*)calloc(strlen(dir)+3, sizeof(u8));
	strcpy(full_folder_dir, dir);
	strcat(full_folder_dir, "\\*");

	u32 file_count = FindNumberOfFilesInDir(full_folder_dir, dot_file_format);

	WIN32_FIND_DATA fd;
	LARGE_INTEGER fs;
	HANDLE h;

	h = FindFirstFile(full_folder_dir, &fd);
	assert(h != INVALID_HANDLE_VALUE);

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

	GenericBuffer file_buffer = {};
	GameAssetFileFormat gaff = {};
	{
		strcpy(gaff.identification, "gaff");
		gaff.number_of_blobs = BLOB_TOTAL;
		//gaff.offset_to_blob_directories
	}
	file_buffer.total_size += sizeof(GameAssetFileFormat);

	Directory directory[BLOB_TOTAL] = {};
	{
		//directory[BLOB_MESHES].offset_to_blob
		//directory[BLOB_MESHES].size_of_blob
		strcpy(directory[BLOB_MESHES].name_of_blob, blob_names[BLOB_MESHES]);
	}
	file_buffer.total_size += sizeof(directory);

	GenericBuffer vb_float_buf = MakeGenericBuffer(float);
	GenericBuffer ib_u32_buf = MakeGenericBuffer(u32);
	GenericBuffer mf_buf = MakeGenericBuffer(MeshFormat);
	GenericBuffer vb_buf = MakeGenericBuffer(VertexBufferFormat);

	{	// Loading Models
		u32 total_meshes = 0;
		u32 total_models = 0;
		u32 total_vertex_buffers = 0;
		char* dir = asset_path_dir[ASSET_TYPE_MODEL];
		char* file_format = asset_file_format[ASSET_TYPE_MODEL];
		FolderInfo folder = LoadFolder(dir, file_format);
		total_models = folder.file_count;

		cgltf_data** cgltf_data_a = (cgltf_data**)calloc(folder.file_count, sizeof(cgltf_data));

		// Loading cgltf data
		for(u8 i=0; i<folder.file_count; i++) {
			FileInfo file = folder.files[i];
			char* path = MakeFullPath(folder.dir, file.name, file.format);

			cgltf_options opt = {};
			assert(cgltf_parse_file(&opt, path, &cgltf_data_a[i]) == cgltf_result_success);
			assert(cgltf_validate(cgltf_data_a[i]) == cgltf_result_success);
			assert(cgltf_load_buffers(&opt, cgltf_data_a[i], path) == cgltf_result_success);

			cgltf_data* data = cgltf_data_a[i];

			// Finding out total no of meshes, vertices, indices
			total_meshes += data->meshes_count;
			for(u8 j=0; j<data->meshes_count; j++) {
				cgltf_mesh* mesh = data->meshes + j;
				assert(data->meshes[j].primitives_count == 1);
				cgltf_primitive*  prim = mesh->primitives;
				assert(prim->type == cgltf_primitive_type_triangles);

				for(u8 k=0; k<prim->attributes_count; k++) {
					cgltf_attribute* att = prim->attributes + k;
					cgltf_accessor* acc = att->data;

					cgltf_size float_count = cgltf_accessor_unpack_floats(acc, NULL, 0);
					switch(att->type) {
						case cgltf_attribute_type_position:
						case cgltf_attribute_type_normal:
						case cgltf_attribute_type_texcoord:
							vb_float_buf.total_size += float_count;
							total_vertex_buffers++;
					}
					assert(prim->indices);
					ib_u32_buf.total_size += prim->indices->count;
				}
			}
		}
		MeshFormat* mesh_format_a = (MeshFormat*)calloc(total_meshes, sizeof(MeshFormat));
		VertexBufferFormat* vertex_buffer_format_a = (VertexBufferFormat*)calloc(total_vertex_buffers, sizeof(VertexBufferFormat));

		// Filling in mesh format and loading vertices and indices
		file_buffer.total_size += sizeof(MeshFormat)*total_meshes; 
		u32 offset_to_vertex_buffer_a = file_buffer.total_size;
		file_buffer.total_size += sizeof(VertexBufferFormat)*total_vertex_buffers;
		u32 offset_to_vertex_buffers_float = file_buffer.total_size;
		file_buffer.total_size += sizeof(float)*vb_float_buf.total_size;
		u32 offset_to_index_buffers_u32 = file_buffer.total_size;

		float* vertex_buffers_float = (float*)calloc(vb_float_buf.total_size, sizeof(float));
		u32* index_buffers_u32 = (u32*)calloc(ib_u32_buf.total_size, sizeof(u32));
		u32 mesh_format_counter = 0;
		u32 vertex_buffer_counter = 0;

		for(u8 i=0; i<total_models; i++) {
			cgltf_data* data = cgltf_data_a[i];

			for(u8 j=0; j<data->meshes_count; j++) {
				cgltf_mesh* mesh = data->meshes + j;
				MeshFormat* mesh_format = mesh_format_a + mesh_format_counter++;
				assert(mesh->name);
				assert(strlen(mesh->name) < STRING_LENGTH_MESH);
				strcpy(mesh_format->mesh_name, mesh->name);
				mesh_format->offset_to_vertex_buffers = offset_to_vertex_buffer_a + sizeof(VertexBufferFormat)*vertex_buffer_counter;

				for(u8 k=0; k<mesh->primitives[0].attributes_count; k++) {
					cgltf_attribute* att = mesh->primitives[0].attributes + k;
					cgltf_accessor* acc = att->data;
					mesh_format->vertices_count = acc->count;
					mesh_format->indices_count = mesh->primitives[0].indices->count;

					mesh_format->offset_to_indices = offset_to_index_buffers_u32;
					offset_to_index_buffers_u32 += mesh_format->indices_count;

					u32* u32_ptr = index_buffers_u32 + index_buffers_u32_counter;
					index_buffers_u32_counter += mesh_format->indices_count;

					cgltf_size float_count = cgltf_accessor_unpack_floats(acc, NULL, 0);
					switch(att->type) {
						case cgltf_attribute_type_position: {
							assert(att->data->component_type == cgltf_component_type_r_32f);
							assert(acc->type == cgltf_type_vec3);
							VertexBufferFormat* vbf = vertex_buffer_format_a + vertex_buffer_counter++;
							strcpy(vbf->type, "POSITION");
							vbf->offset_to_data = offset_to_vertex_buffer_float;

							float* float_ptr = vertex_buffers_float + vertex_buffers_float_counter;
							cgltf_accessor_unpack_floats(acc, float_ptr, float_count);
							vertex_buffers_float_counter += float_count;
							offset_to_vertex_buffer_float += float_count;

							mesh_format->vertex_buffer_count++;
						} break;

						case cgltf_attribute_type_normal: {
							assert(att->data->component_type == cgltf_component_type_r_32f);
							assert(acc->type == cgltf_type_vec3);
							VertexBufferFormat* vbf = vertex_buffer_format_a + vertex_buffer_counter++;
							strcpy(vbf->type, "NORMAL");
							vbf->offset_to_data = offset_to_vertex_buffer_float;

							float* float_ptr = vertex_buffers_float + vertex_buffers_float_counter;
							cgltf_accessor_unpack_floats(acc, float_ptr, float_count);
							vertex_buffers_float_counter += float_count;
							offset_to_vertex_buffer_float += float_count;

							mesh_format->vertex_buffer_count++;
						} break;

						case cgltf_attribute_type_texcoord: {
							assert(att->data->component_type == cgltf_component_type_r_32f);
							assert(acc->type == cgltf_type_vec2);
							VertexBufferFormat* vbf = vertex_buffer_format_a + vertex_buffer_counter++;
							strcpy(vbf->type, "TEXCOORD");

							vbf->offset_to_data = offset_to_vertex_buffer_float;
							
							float* float_ptr = vertex_buffers_float + vertex_buffers_float_counter;
							cgltf_accessor_unpack_floats(acc, float_ptr, float_count);
							vertex_buffers_float_counter += float_count;
							offset_to_vertex_buffer_float += float_count;

							mesh_format->vertex_buffer_count++;
						} break;
					}
				}
			}
		}
	}
	file_buffer.total_size = sizeof(float)*vb_float_buf.total_size + sizeof(u32)*ib_u32_buf.total_size;


	return 1;
}



// TODO: Split structs into seprate files
