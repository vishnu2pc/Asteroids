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

#define PushStruct(ptr_arena, type) (type*)GetMemory((ptr_arena), sizeof(type))
#define PushArray(ptr_arena, type, count) (type*)GetMemory((ptr_arena), sizeof(type)*(count))

#define PopStruct(ptr_arena, type) (type*)FreeMemory((ptr_arena), sizeof(type))
#define PopArray(ptr_arena, type, count) FreeMemory((ptr_arena), sizeof(type)*(count))

struct Memory {
	u8* bp;
	u32 size;
};

struct MemoryArena {
	Memory memory;
	u32 used;
};

static u8* GetMemory(MemoryArena* arena, u32 size) {
	u8* ptr = arena->memory.bp + arena->used;
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											// Bitwise op to get next rounded multiple of 8
	arena->used += byte_aligned_size;
	Assert(arena->used < arena->memory.size);
	return ptr;
}

static void FreeMemory(MemoryArena* memory, u32 size) {
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											
	Assert(memory->used - size >= 0);
	memory->used -= byte_aligned_size;
}

static MemoryArena ExtractMemoryArena(MemoryArena* arena, u32 size) {
	MemoryArena result = {};
	result.memory.bp = GetMemory(arena, size);
	result.memory.size = size;
	return result;
}

u32 StringLength(char* string) {
	u32 i=0;
	while(string[i] != 0) i++;
	return i;
}

bool StringCompare(char* left, char* right) {
	u32 left_len = StringLength(left);
	u32 right_len = StringLength(right);
	if(left_len != right_len) return false;
	for(u32 i=0; left[i] != 0; i++)
		if(left[i] != right[i]) return false;
	return true;
}


