struct GenericBuffer {
	void* data;
	u32 elem_size;
	u32 total_size;
	u32 allocated_size;
};

#define MakeGenericBuffer(type) (MakeGenericBufferFull(0, sizeof(type), 0, 0))

GenericBuffer MakeGenericBufferFull(void* data, u32 elem_size, u32 total_size, u32 allocated_size) {
	return GenericBuffer { data, elem_size, total_size, allocated_size };
}

