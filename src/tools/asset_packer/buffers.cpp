struct StructBuffer {
	void* data;
	u32 elem_size;
	u32 total_count;
	u32 filled_count;
};

static u32 GetFilledSizeStructBuffer(StructBuffer* sb) {
	return sb->filled_count*sb->elem_size;
}

static void* GetElementStructBuffer(StructBuffer* sb, u32 index) {
	assert(index < sb->filled_count);
	return (u8*)sb->data + sb->elem_size*index;
}

static u32 GetOffsetStructBuffer(StructBuffer* sb) {
	return sb->elem_size*sb->filled_count;
}

static void* GetCursorStructBuffer(StructBuffer* sb) {
	return (u8*)sb->data + sb->elem_size*sb->filled_count;
}

StructBuffer MakeStructBuffer(u32 elem_size) {
	StructBuffer result = {};
	result.data = calloc(1, elem_size);
	result.elem_size = elem_size;
	result.total_count = 1;
	result.filled_count = 0;
	return result;
}

static void ReserveMemoryStructBuffer(u32 count, StructBuffer* sb) {
	if(sb->filled_count+count >= sb->total_count) {
		u32 new_count = 2 * sb->total_count + count;
		u32 new_size = sb->elem_size * new_count;
		sb->data = (void*)realloc(sb->data, new_size);
		sb->total_count = new_count;
		assert(sb->data);
	}
}

// returns offset before pushing
static u32 PushStructBuffer(void* element, u32 count, StructBuffer* sb) {
	u32 offset = GetOffsetStructBuffer(sb);
	ReserveMemoryStructBuffer(count, sb);
	void* ptr = (u8*)sb->data + sb->elem_size*sb->filled_count;
	memcpy(ptr, element, sb->elem_size*count);
	sb->filled_count += count;
	return offset;
}

struct GenericBuffer {
	void* data;
	u32 total_size;
	u32 filled_size;
};

static void* GetCursorGenericBuffer(GenericBuffer* gb) {
	return (u8*)gb->data + gb->filled_size;
}

static u32 PushGenericBuffer(void* data, u32 size, GenericBuffer* gb) {
	assert(gb->total_size - gb->filled_size >= size);
	memcpy(GetCursorGenericBuffer(gb), data, size); 
	gb->filled_size += size;
}

static void WriteStructBufferToGenericBuffer(GenericBuffer* gb, StructBuffer* sb) {
	void* gb_ptr = GetCursorGenericBuffer(gb);
	void* sb_ptr = sb->data;

	u32 size = sb->elem_size * sb->filled_count;
	memcpy(gb_ptr, sb_ptr, size);
	gb->filled_size += size;
}

static void WriteStructBufferElementToGenericBuffer(GenericBuffer* gb, StructBuffer* sb, u32 index) {
	void* gb_ptr = GetCursorGenericBuffer(gb);
	void* sb_ptr = (u8*)sb->data + sb->elem_size*index;

	memcpy(gb_ptr, sb_ptr, sb->elem_size);
	gb->filled_size += sb->elem_size;
}
