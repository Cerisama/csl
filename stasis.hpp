// Stasis
csl_str csl_stasis_name(csl_str name) {
	csl_str a = csl_str_concat((csl_str)"\x0E\x00\x00\x00.DO_NOT_SHARE.", name);
	csl_str b = csl_str_concat(a, (csl_str)"\x07\x00\x00\x00.stasis");
	csl_str_destroy(a);
	return b;
}
ex void csl_stasis_freeze(u32 ver, csl_str name) {
	name = csl_stasis_name(name);
	csl_fiber_shutdown();
	fs f = csl_fs_create(name, 2);
	csl_globals->stasis_fs = csl_vaddr_store(f);
	csl_str_destroy(name);
	csl_fs_writemem(f, &ver, sizeof(u32));
	u32 vs = *(u32*)vaddrres;
	csl_vaddr va = csl_vaddr_store(csl_globals);
	csl_fs_writemem(f, &va, sizeof(csl_vaddr));
	for (u32 i = 0; i < vs; i++) {
		void* mp = *(void**)((char*)vaddrres + 8 + (i * sizeof(void*)));
		if (mp != 0) {
			u32 blocksize = *(u32*)((char*)mp);
			u32 blocks = *(u32*)((char*)mp + 4);
			u32 pagesize = 8 + (blocks / 8) + (blocksize * blocks);
			csl_fs_writemem(f, &pagesize, sizeof(u32));
			csl_fs_writemem(f, mp, pagesize);
		}
	}
	csl_program_abort(0);
}
ex bool csl_stasis_resume(u32 ver, csl_str name) {
	name = csl_stasis_name(name);
	fs f = csl_fs_create(name, 0);
	csl_str_destroy(name);
	if (f == 0) {
		csl_program_init();
		return false;
	}
	u32 ver2 = 0;
	csl_fs_readmem(f, &ver2, sizeof(u32));
	if (ver != ver2) {
		csl_program_init();
		return false;
	}
	u64 l = csl_fs_size(f);
	u32 sz = *((u32*)mp);
	for (u32 i = 0; i < sz; i++) {
		csl_mem_deletepage(*((u32*)mp) - 1);
	}
	csl_vaddr_reset();
	csl_vaddr va = 0;
	csl_fs_readmem(f, &va, sizeof(csl_vaddr));
	u64 ptr = sizeof(csl_vaddr) + 4;
	while (ptr < l) {
		u32 ms = 0;
		csl_fs_readmem(f, &ms, sizeof(u32));
		csl_mempage mp = csl_mem_createpage(csl_mem_blocksize, csl_mem_blocks);
		csl_fs_readmem(f, mp, ms);
		csl_mem_pushpage(mp);
		ptr += sizeof(u32) + ms;
	}
	csl_globals = (csl_global_type*)csl_vaddr_get(va);
	csl_mem_free(csl_vaddr_get(csl_globals->stasis_fs));
	csl_program_init();
	return true;
}