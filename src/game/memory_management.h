#define ZeroStruct(val) ZeroMem(&(val), sizeof(val))
#define ZeroArray(array, count) ZeroMem(array, count*sizeof((array)[0]))

static void ZeroMem(void* ptr, u64 size) {
	u8* mem = (u8*)ptr;
	while(size--) *mem++ = 0;
}

static void CopyMem(void* to, void* from, u64 len) {
	u8* src = (u8*)from;
	u8* dst = (u8*)to;
	while(len--) *dst++ = *src++;
}

static bool CompareMem(void* left, void* right, u64 len) {
	u8* l = (u8*)left;
	u8* r = (u8*)right;
	while(len--) if(*l++ != *r++) return false;
	return true;
}

#define PushSize(ptr_arena, size) PushSize_((ptr_arena), (size))
#define PushStruct(ptr_arena, type) (type*)PushSize_((ptr_arena), sizeof(type))
#define PushArray(ptr_arena, type, count) (type*)PushSize_((ptr_arena), sizeof(type)*(count))
#define BootstrapPushStruct(type, member, min_size) (type*)BootstrapPushSize_(sizeof(type), offsetof(type, member), min_size)

struct MemoryArena {
	PlatformMemoryBlock* current_block;
	u64 min_block_size;
	u32 temp_count;
};

struct TemporaryMemory {
	MemoryArena* arena;
	PlatformMemoryBlock* block;
	u64 used;
};

static void*
PushSize_(MemoryArena* arena, u64 size) {
	void* result = 0;

	u32 aligned_size = ((size + 7) & (-8));
	if(!arena->current_block || ((arena->current_block->used + aligned_size) > arena->current_block->size)) {
		if(!arena->min_block_size) arena->min_block_size = 1024*1024;

		u64 block_size = Max(aligned_size, arena->min_block_size);
		PlatformMemoryBlock* new_block = platform_api.allocate_memory(block_size);
		new_block->prev = arena->current_block;
		arena->current_block = new_block;
	}

	Assert((arena->current_block->used + aligned_size) <= arena->current_block->size);

	result = arena->current_block->bp + arena->current_block->used;
	arena->current_block->used += aligned_size;

	return result;
}

static TemporaryMemory 
BeginTemporaryMemory(MemoryArena* arena) {
	TemporaryMemory result = {};

	result.arena = arena;
	result.block = arena->current_block;
	result.used = arena->current_block ? arena->current_block->used : 0;

	arena->temp_count++;

	return result;
}

static void
FreeLastBlock(MemoryArena* arena) {
	PlatformMemoryBlock* free_block = arena->current_block;
	arena->current_block = free_block->prev;
	platform_api.deallocate_memory(free_block);
}

static void
EndTemporaryMemory(TemporaryMemory* temp_mem) {
	MemoryArena* arena = temp_mem->arena;

	while(arena->current_block != temp_mem->block) {
		FreeLastBlock(arena);
	}

	if(arena->current_block) {
		Assert(arena->current_block->used >= temp_mem->used);
		arena->current_block->used = temp_mem->used;
	}

	Assert(arena->temp_count > 0);
	arena->temp_count--;
}

static void
ClearMemoryArena(MemoryArena* arena) {
	while(arena->current_block) {
		bool last_block = arena->current_block->prev == 0;
		FreeLastBlock(arena);
		if(last_block) break;
	}
}

static void
CheckArena(MemoryArena* arena) {
	Assert(arena->temp_count == 0);
}

static void*
BootstrapPushSize_(u64 struct_size, u64 offset_to_arena, u64 min_block_size) {
	MemoryArena bootstrap = {};
	bootstrap.min_block_size = min_block_size;
	void* structure = PushSize_(&bootstrap, struct_size);
	*(MemoryArena*)((u8*)structure + offset_to_arena) = bootstrap;

	return structure;
}

static u32
StringLength(char* string) {
	u32 i=0;
	while(string[i] != 0) i++;
	return i;
}

static bool
StringCompare(char* left, char* right) {
	u32 left_len = StringLength(left);
	u32 right_len = StringLength(right);
	if(left_len != right_len) return false;
	for(u32 i=0; left[i] != 0; i++)
		if(left[i] != right[i]) return false;
	return true;
}


