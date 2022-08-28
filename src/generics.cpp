typedef u32 Pair;

static u16 GetUpper(u32 pair) {
	return (u16)(pair >> 16);
};

static u16 GetLower(u32 pair) {
	return (u16)pair;
};

static void SetUpper(u32* pair, u16 value) {
	u32 mask = 0x0000FFFF;
	*pair = (*pair & mask) | (value << 16);
};

static void SetLower(u32* pair, u16 value) {
	u32 mask = 0xFFFF0000;
	*pair = (*pair & mask) | (u32)value;
};

static u32 BitsToMaxValue(u8 bit_count) {
	return (u32)(((u64)1 << bit_count) - 1);
}

static u64 SetBits(u64 handle, u8 lsb, u8 length, u32 value) {
	u64 result = 0;

	assert(lsb + length <= 64);
	u64 mask = ~((((u64)1 << length) - 1) << lsb);

	result = (handle & mask) | value;
}

static u32 GetBits(u64 handle, u8 lsb, u8 length) {
	u32 result = 0;

	assert(lsb + length <= 64);
	u64 mask = ~((((u64)1 << length) - 1) << lsb);

	result = (handle & mask) >> length;
}
