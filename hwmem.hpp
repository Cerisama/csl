// HW memory
void* csl_mem_hwalloc(u32 size) {
	void* al = malloc(size);
	return al;
}
void csl_mem_hwfree(void* al) {
	free(al);
}
ex inline i8 csl_mem_aligned(csl_allocation dst, csl_allocation src, u32 alignment) {
	i8 a = ((ptr)src & (alignment - 1));
	i8 b = ((ptr)dst & (alignment - 1));
	if (a == b) {
		if (a == 0) {
			return 0;
		}
		else {
			return alignment - a;
		}
	}
	else {
		return -1;
	}
}
void csl_mem__cpy(csl_allocation dst, csl_allocation src, u32 size, bool m) {
	i8 al8 = csl_mem_aligned(dst, src, 8);
	i8 al4 = csl_mem_aligned(dst, src, 4);
	i8 al2 = csl_mem_aligned(dst, src, 2);
	u32 go = 0;
	i8 use = 0;
	if (al8 != -1) {
		go = al8;
		use = 8;
	}
	else if (al4 != -1) {
		go = al4;
		use = 4;
	}
	else if (al2 != -1) {
		go = al2;
		use = 2;
	}
	else {
		go = 0;
		use = 1;
	}
	for (u32 i = 0; i < go; i++) {
		u32 a = i;
		if (m) {
			a = 0;
		}
		*(u8*)((char*)dst + i) = *(u8*)((char*)src + a);
	}
	while (true) {
		if (go + use > size) {
			if (use == 1) {
				break;
			}
			use /= 2;
			continue;
		}
		u32 a = go;
		if (m) {
			a = 0;
		}
		if (use == 8) {
			*(u64*)((char*)dst + go) = *(u64*)((char*)src + a);
		}
		else if (use == 4) {
			*(u32*)((char*)dst + go) = *(u32*)((char*)src + a);
		}
		else if (use == 2) {
			*(u16*)((char*)dst + go) = *(u16*)((char*)src + a);
		}
		else if (use == 1) {
			*(u8*)((char*)dst + go) = *(u8*)((char*)src + a);
		}
		go += use;
	}
}
ex void csl_mem_cpy(csl_allocation dst, csl_allocation src, u32 size) {
	if (size > 0) {
		if (usecrt) {
			memcpy((void*)dst, (void*)src, size);
		}
		else {
			csl_mem__cpy(dst, src, size, false);
		}
	}
}
ex void csl_mem_set(void* dst, u32 size, u8 val) {
	u32 v = val;
	u32 bval = (v >> 24) | (v >> 16) | (v >> 8) | v;
	if (size > 0) {
		if (usecrt) {
			memset((void*)dst, bval, size);
		}
		else {
			u64 bval2 = (u64)bval | ((u64)bval >> 32);
			csl_mem__cpy(dst, &bval2, size, true);
		}
	}
}