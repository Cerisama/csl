// Memory
csl_mutex mm = 0;
u64 lastpage = 0;
u64 lastblock = 0;
csl_mempageptr mp = nullptr;
void csl_mem_pushpage(csl_mempage pmp) {
	u32 size = 0;
	if (mp != nullptr) {
		size = *((u32*)mp);
	}
	size++;
	csl_mempageptr nmp = csl_mem_hwalloc(4 + (size * sizeof(void*)));
	*((u32*)nmp) = size;
	if (mp != nullptr) {
		csl_mem_cpy((char*)nmp + 4, (char*)mp + 4, (size - 1) * sizeof(void*));
	}
	*(void**)((char*)nmp + 4 + ((size - 1) * sizeof(void*))) = pmp;
	csl_mem_hwfree(mp);
	mp = nmp;
}
ex void csl_mem_zero(void* ptr, u32 size) {
	csl_mem_set(ptr, size, 0x00);
}
csl_mempage csl_mem_createpage(u32 blocksize, u32 blocks) {
	u64 pagesize = 8 + (blocks / 8) + (blocksize * blocks);
	csl_mempage tmp = csl_mem_hwalloc(pagesize);
	csl_vaddr_add(tmp);
	csl_mem_zero((void*)((char*)tmp + 4), blocks / 8);
	*((u32*)tmp) = blocksize;
	*((u32*)((char*)tmp + 4)) = blocks;
	return tmp;
}
#define csl_mem_blocks 16384
#define csl_mem_blocksize 32
csl_mempage csl_mem_getpage(u32 n) {
	if (csl_globals != 0) {
		if (csl_globals->del == n + 1) {
			csl_globals->del = 0;
		}
	}
	bool cnew = mp == nullptr;
	if (!cnew) {
		cnew = n + 1 > *((u32*)mp);
	}
	if (cnew) {
		csl_mem_pushpage(csl_mem_createpage(csl_mem_blocksize, csl_mem_blocks));
	}
	return *((csl_mempage*)((char*)mp + (sizeof(void*) * n) + 4));
}
void csl_mem_deletepage(u32 n) {
	u32 size = *((u32*)mp) - 1;
	csl_mempageptr nmp = csl_mem_hwalloc(4 + (size * sizeof(void*)));
	csl_vaddr_remove(csl_mem_getpage(n));
	*((u32*)nmp) = size;
	if (n != 0) {
		csl_mem_cpy((char*)nmp + 4, (char*)mp + 4, n * sizeof(void*));
	}
	if (n < size) {
		csl_mem_cpy((char*)nmp + 4 + (sizeof(void*) * n), (char*)mp + 4 + (sizeof(void*) * (n + 1)), (size - n - 1) * sizeof(void*));
	}
	csl_mem_hwfree(mp);
	mp = nmp;
}
void csl_mem_checkdelete() {
	if (csl_globals != 0) {
		if (csl_globals->del != 0) {
			csl_mem_deletepage(csl_globals->del - 1);
			csl_globals->del = 0;
		}
	}
}
void csl_mem_markdelete(u32 n) {
	if (csl_globals != 0) {
		csl_mem_checkdelete();
		csl_globals->del = n + 1;
	}
}
ex bool csl_mem_getbit(csl_allocation src, u32 offs) {
	src = (char*)src + (offs / 8);
	u8 bii = offs % 8;
	return ((*(u8*)src) >> bii) & 1;
}
ex void csl_mem_setbit(csl_allocation src, u32 offs, bool val) {
	src = (char*)src + (offs / 8);
	u8 bii = offs % 8;
	if (val) {
		*(u8*)src |= (1u << bii);
	}
	else {
		*(u8*)src &= ~(1u << bii);
	}
}
csl_allocation csl_mem__alloc(u32 size) {
	if (size > 32768 || usecrt) { // 32kb
		size += sizeof(u32) + sizeof(u32) + 4;
		csl_allocation tmp = csl_mem_hwalloc(size);
		tmp = (char*)tmp + 8;
		*(u32*)((char*)tmp - 12) = 0;
		*(u32*)((char*)tmp - 4) = size - 8;
		return tmp;
	}
	else {
		csl_mutex_acquire(&mm);
		size += sizeof(void*) + sizeof(u32);
		u32 v = lastpage;
		while (true) {
			csl_mempage pg = csl_mem_getpage(v);
			u32 blocksize = *((u32*)pg);
			u32 blocks = *((u32*)((char*)pg + 4));
			u32 reqb = (size + (blocksize - 1)) / blocksize;
			bool found = false;
			u32 ui = 0;
			u32 sbl = 0;
			if (v == lastpage) {
				sbl = lastblock;
			}
			for (u32 i = sbl; i < blocks; i++) {
				bool freeblock = !csl_mem_getbit((char*)pg + 12, i);
				if (freeblock) {
					ui++;
					if (ui == reqb) {
						ui = i;
						found = true;
						break;
					}
				}
				else {
					ui = 0;
				}
			}
			if (found) {
				lastpage = v;
				ui -= reqb - 1;
				lastblock = ui + reqb;
				*(u32*)((char*)pg + 8) += reqb;
				for (u32 a = ui; a < ui + reqb; a++) {
					csl_mem_setbit((char*)pg + 12, a, 1);
				}
				csl_allocation al = ((char*)pg + sizeof(void*) + 4 + 12 + (blocks / 8) + (ui * blocksize));
				csl_vaddr va = csl_vaddr_store(pg);
				*(csl_vaddr*)((char*)al - 8) = va;
				*(u32*)((char*)al - 12) = reqb;
				csl_mutex_release(&mm);
				return al;
			}
			v++;
		}
	}
}
ex u32 csl_mem_size(csl_allocation ptr) {
	u32 reqb = *(u32*)((char*)ptr - 12);
	if (reqb != 0) {
		csl_mempage pg = csl_vaddr_get(*(csl_vaddr*)((char*)ptr - 8));
		u32 blocksize = *((u32*)pg);
		return (reqb * blocksize) - sizeof(csl_mempage) - sizeof(u32);
	}
	else {
		return *(u32*)((char*)ptr - (sizeof(u32) * 2));
	}
}
//#define csl_mem_alloc(size) csl_mem_allocX(size, __FILE__)
ex csl_allocation csl_mem_alloc(u32 size) {
	csl_allocation ma = csl_mem__alloc(size);
	//std::cout << "A " << ma << " : " << l << "\n"; , const char* l
	csl_mem_checkdelete();
	return ma;
}
//#define csl_mem_zalloc(size) csl_mem_zallocX(size, __FILE__)
ex csl_allocation csl_mem_zalloc(u32 size) {
	csl_allocation ma = csl_mem__alloc(size);
	//std::cout << "Z " << ma << " : " << l << "\n";
	csl_mem_zero(ma, csl_mem_size(ma));
	csl_mem_checkdelete();
	return ma;
}
//#define csl_mem_free(ptr) csl_mem_freeX(ptr, __FILE__); 
ex void csl_mem_free(csl_allocation ptr) {
	//std::cout << "F " << ptr << " : " << l << "\n";
	u32 reqb = *(u32*)((char*)ptr - 12);
	if (reqb == 0) {
		csl_mem_hwfree((char*)ptr - 12);
		return;
	}
	csl_mempage pg = csl_vaddr_get(*(csl_vaddr*)((char*)ptr - 8));
	csl_mutex_acquire(&mm);
	u32 blocksize = *((u32*)pg);
	u32 blocks = *((u32*)((char*)pg + 4));
	*((u32*)((char*)pg + 8)) -= reqb;
	u32 i = 0;
	while (true) {
		if (*(char**)((char*)mp + 4 + (i * sizeof(void*))) == pg) {
			break;
		}
		i++;
	}
	if (*((u32*)((char*)pg + 8)) == 0) {
		csl_mem_markdelete(i);
		lastpage = 0;
		lastblock = 0;
		csl_mutex_release(&mm);
		return;
	}
	u32 ui = ((char*)ptr - pg - (blocks / 8) - 8) / blocksize;
	if (i < lastpage || (ui + reqb < lastblock && i == lastpage)) {
		lastpage = i;
	}
	lastblock = ui;
	for (u32 a = ui; a < ui + reqb; a++) {
		csl_mem_setbit((char*)pg + 12, a, 0);
	}
	csl_mutex_release(&mm);
}