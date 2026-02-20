// List
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
// NEED SPLITTING
#define gfl(l, isz, n) (void*)((char*)l + sizeof(void*) + (2*sizeof(u32)) + (isz * (n)))
void csl_listfragment_create(csl_listfragment prev, u32 len) {
	u32 isz = *(u32*)((char*)prev + sizeof(csl_vaddr) + sizeof(u32));
	u32 size = (len * isz) + sizeof(csl_vaddr) + sizeof(u32) + sizeof(u32);
	csl_listfragment all = csl_mem_alloc(size);
	*(csl_vaddr*)((char*)all) = 0;
	*(u32*)((char*)all + sizeof(csl_vaddr)) = len;
	*(u32*)((char*)all + sizeof(csl_vaddr) + sizeof(u32)) = isz;
	*(csl_vaddr*)((char*)prev) = csl_vaddr_store(all);
}
void csl_listfragment_replacenext(csl_listfragment prev, u32 place, i32 ielen) {
	csl_vaddr nxt = *(csl_vaddr*)((char*)prev);
	if (nxt == 0) {
		csl_listfragment_create(prev, place + ielen);
		return;
	}
	csl_listfragment oldl = csl_vaddr_get(nxt);
	csl_vaddr nextl = *(csl_vaddr*)((char*)oldl);
	u32 len = ((i32) * (u32*)((char*)oldl + sizeof(csl_vaddr))) + ielen;
	if (len == 0) {
		*(csl_vaddr*)((char*)prev) = nextl;
	}
	else {
		u32 isz = *(u32*)((char*)oldl + sizeof(csl_vaddr) + sizeof(u32));
		u32 size = (len * isz) + sizeof(csl_vaddr) + sizeof(u32) + sizeof(u32);
		csl_listfragment newl = csl_mem_alloc(size);
		*(csl_vaddr*)((char*)newl) = nextl;
		*(u32*)((char*)newl + sizeof(csl_vaddr)) = len;
		*(u32*)((char*)newl + sizeof(csl_vaddr) + sizeof(u32)) = isz;
		csl_mem_cpy(gfl(newl, isz, 0), gfl(oldl, isz, 0), place * isz);
		if (ielen > 0) {
			csl_mem_cpy(gfl(newl, isz, place + ielen), gfl(oldl, isz, place), (len - ielen - place) * isz);
		}
		else {
			csl_mem_cpy(gfl(newl, isz, place), gfl(oldl, isz, place - ielen), (len - place) * isz);
		}
		*(csl_vaddr*)((char*)prev) = csl_vaddr_store(newl);
	}
	csl_mem_free(oldl);
}
ex csl_list csl_list_create(u32 isz) {
	u32 size = sizeof(csl_vaddr) + sizeof(u32) + sizeof(u32);
	csl_list l = csl_mem_zalloc(size);
	*(u32*)((char*)l + sizeof(csl_vaddr) + sizeof(u32)) = isz;
	return l;
}
void* csl_list_replace(csl_list l, u32 pos, i32 am, void* obj = 0) {
	u32 isz = *(u32*)((char*)l + sizeof(csl_vaddr) + sizeof(u32));
	csl_listfragment lf = l;
	while (true) {
		csl_vaddr nxt = *(csl_vaddr*)((char*)lf);
		if (nxt == 0) {
			break;
		}
		csl_listfragment next = csl_vaddr_get(nxt);
		u32 l = *(u32*)((char*)next + sizeof(csl_vaddr));
		if ((i32)pos - (i32)l < 0) {
			break;
		}
		lf = next;
		pos -= l;
	}
	if (am != 0) {
		csl_listfragment_replacenext(lf, pos, am);
	}
	csl_vaddr nxt = *(csl_vaddr*)((char*)lf);
	csl_listfragment next = 0;
	if (nxt != 0) {
		next = csl_vaddr_get(*(csl_vaddr*)((char*)lf));
	}
	if (obj != 0) {
		csl_mem_cpy(gfl(next, isz, pos), obj, isz);
	}
	return gfl(next, isz, pos);
}
ex void csl_list_set(csl_list l, u32 pos, void* obj) {
	csl_list_replace(l, pos, 0, obj);
}
ex void csl_list_insert(csl_list l, u32 pos, void* obj) {
	csl_list_replace(l, pos, 1);
	csl_list_set(l, pos, obj);
}
ex void csl_list_erase(csl_list l, u32 pos) {
	csl_list_replace(l, pos, -1);
}
ex void* csl_list_get(csl_list l, u32 pos) {
	return csl_list_replace(l, pos, 0);
}
ex u32 csl_list_size(csl_list l) {
	u32 sz = 0;
	while (true) {
		csl_vaddr nxt = *(csl_vaddr*)((char*)l);
		if (nxt == 0) {
			break;
		}
		csl_listfragment next = csl_vaddr_get(nxt);
		sz += *(u32*)((char*)next + sizeof(csl_vaddr));
		l = next;
	}
	return sz;
}
ex u32 csl_list_objsize(csl_list l) {
	return *(u32*)((char*)l + sizeof(csl_vaddr) + sizeof(u32));
}
ex void csl_list_append(csl_list l, void* obj) {
	csl_list_insert(l, csl_list_size(l), obj);
}
ex void csl_list_destroy(csl_list l) {
	while (true) {
		csl_vaddr nxt = *(csl_vaddr*)((char*)l);
		csl_mem_free(l);
		if (nxt == 0) {
			break;
		}
		l = csl_vaddr_get(*(csl_vaddr*)((char*)l));
	}
}