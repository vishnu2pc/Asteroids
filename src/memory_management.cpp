struct Memory {
	u8* bp;
	u32 size;
};

struct MemoryArena {
	Memory mem;
	u32 filled;
};

#define PushStruct(ptr_arena, type) (type*)GetMemory((ptr_arena), sizeof(type))
#define PushArray(ptr_arena, type, count) (type*)GetMemory((ptr_arena), sizeof(type)*(count))

#define PopStruct(ptr_arena, type) (type*)FreeMemory((ptr_arena), sizeof(type))
#define PopArray(ptr_arena, type, count) FreeMemory((ptr_arena), sizeof(type)*(count))

static Memory AllocateMemory(u32 size) {
	Memory mem = {};
	mem.bp = (u8*)calloc(1, size);
	mem.size = size;
	return mem;
}

static u8* GetMemory(MemoryArena* arena, u32 size) {
	u8* ptr = arena->mem.bp + arena->filled;
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											// Bitwise op to get next rounded multiple of 8
	arena->filled += byte_aligned_size;
	assert(arena->filled + byte_aligned_size < arena->mem.size);
	return ptr;
}

static void FreeMemory(MemoryArena* mem, u32 size) {
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											
	assert(mem->filled - size >= 0);
	mem->filled -= byte_aligned_size;
}

static MemoryArena ExtractMemoryArena(MemoryArena* arena, u32 size) {
	MemoryArena result = {};
	result.mem.bp = GetMemory(arena, size);
	result.mem.size = size;
	return result;
}
