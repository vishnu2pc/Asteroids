struct GlobalMemory {
	void* bp;
	u64 total_size;
	u64 allocated_size;
	u64 frame_begin_size;
};

static GlobalMemory GM;
static GlobalMemory SM;

#define AllocateMasterMemory(size) AllocateGlobalMemory(&GM, size)
#define PushMaster(type, count) (type*)GetMemory(&GM, sizeof(type)*(count))
#define PopMaster(type, count) FreeMemory(&GM, sizeof(type)*count)

#define AllocateScratchMemory(size) AllocateGlobalMemory(&SM, size)
#define PushScratch(type, count) (type*)GetMemory(&SM, sizeof(type)*(count))
#define PopScratch(type, count) FreeMemory(&SM, sizeof(type)*count)

static void AllocateGlobalMemory(GlobalMemory* gm, u32 size) {
	gm->bp = calloc(1, size);
	gm->total_size = size;
}

static void* GetMemory(GlobalMemory* gm, u32 size) {
	void* ptr = (void*)((char*)gm->bp + gm->allocated_size);
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											// Bitwise op to get next rounded multiple of 8
	gm->allocated_size += byte_aligned_size;
	assert(gm->allocated_size + byte_aligned_size < gm->total_size);
	return ptr;
}

static void FreeMemory(GlobalMemory* gm, u32 size) {
	u32 byte_aligned_size = ((size + 7) >> 3) << 3;											
	assert(gm->allocated_size - size >= 0);
	gm->allocated_size -= byte_aligned_size;
}

static void BeginMemoryCheck() {
	SM.frame_begin_size = SM.allocated_size;
}

static void EndMemoryCheck() {
	assert(SM.frame_begin_size == SM.allocated_size);
}