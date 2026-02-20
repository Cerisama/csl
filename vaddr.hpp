// vaddr
// List: [32:size][32:none][64*n:addresses]
// VADDR: [32:block][32:offs]
csl_allocation vaddrres = 0;
void csl_vaddr_csize(u32 size) {
	u32 osz = 0;
	if (vaddrres != 0) {
		osz = *(u32*)vaddrres;
	}
	u32 alsz = sizeof(u32) + 4 + (size * sizeof(void*));
	csl_allocation al = csl_mem_hwalloc(alsz);
	if (osz != 0) {
		csl_mem_cpy((char*)al + 8, (char*)vaddrres + 8, osz * sizeof(void*));
		csl_mem_hwfree(vaddrres);
	}
	*(u32*)al = size;
	vaddrres = al;
}
void csl_vaddr_reset() {
	csl_mem_hwfree(vaddrres);
	vaddrres = 0;
}
csl_vaddr csl_vaddr_merge(u32 block, u32 offs) {
	csl_vaddr addr = 0;
	*(u32*)((char*)&addr) = block;
	*(u32*)((char*)&addr + 4) = offs;
	return addr;
}
u32 vaddrs = 0;
csl_vaddr csl_vaddr_add(void* val) {
	u32 sz = 1;
	if (vaddrres != 0) {
		sz = *(u32*)vaddrres + 1;
	}
	for (u32 i = vaddrs; i < sz - 1; i++) {
		void* n = *(void**)((char*)vaddrres + 8 + (i * sizeof(void*)));
		if (n == 0) {
			*(void**)((char*)vaddrres + 8 + (i * sizeof(void*))) = val;
			vaddrs = i + 1;
			return csl_vaddr_merge(i, 0);
		}
	}
	csl_vaddr_csize(sz);
	*(void**)((char*)vaddrres + 8 + ((sz - 1) * sizeof(void*))) = val;
	return csl_vaddr_merge(sz - 1, 0);
}
void csl_vaddr_remove(void* val) {
	u32 i = 0;
	while (true) {
		void* n = *(void**)((char*)vaddrres + 8 + (i * sizeof(void*)));
		if (n == val) {
			*(void**)((char*)vaddrres + 8 + (i * sizeof(void*))) = 0;
			if (i < vaddrs) {
				vaddrs = i;
			}
			return;
		}
		i++;
	}
}
ex csl_vaddr csl_vaddr_store(void* ptr) {
	u32 sz = *(u32*)vaddrres;
	u32 lblock = 0;
	u32 ldist = -1;
	for (u32 i = 0; i < sz; i++) {
		char* bptr = *(char**)((char*)vaddrres + 8 + (i * sizeof(void*)));
		if (ptr >= bptr) {
			u32 dist = (char*)ptr - bptr;
			if (dist < ldist) {
				lblock = i;
				ldist = dist;
			}
		}
	}
	csl_vaddr v = csl_vaddr_merge(lblock + 1, ldist);
	return v;
}
ex void* csl_vaddr_get(csl_vaddr vaddr) {
	u32 block = *(u32*)((char*)&vaddr);
	u32 offs = *(u32*)((char*)&vaddr + 4);
	if (block == 0 && offs == 0) {
		return 0;
	}
	block -= 1;
	void* v = *(char**)((char*)vaddrres + 8 + (block * sizeof(void*))) + offs;
	return v;
}