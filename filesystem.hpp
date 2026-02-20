// Filesystem

ex void csl_fs_flush(fs f) {
	fflush(*(fptr*)(f));
}
ex void csl_fs_move(fs f, u64 pos) {
	fseek(*(fptr*)(f), pos, 0);
}
ex u64 csl_fs_cur(fs f) {
	return ftell(*(fptr*)(f));
}
ex u64 csl_fs_size(fs f) {
	return *(u64*)((char*)f + sizeof(fptr));
}
ex void csl_fs_write(fs f, csl_str str) {
	u32 sz = csl_str_size(str);
	fwrite(str + 4, 1, sz, *(fptr*)(f));
	if (sz == *(u64*)((char*)f + sizeof(fptr))) { // look back to this?
		*(u64*)((char*)f + sizeof(fptr)) += sz;
	}
	csl_fs_flush(f);
}
ex void csl_fs_writemem(fs f, csl_allocation str, u32 size) {
	fwrite(str, 1, size, *(fptr*)(f));
	if (size == *(u64*)((char*)f + sizeof(fptr))) { // look back to this?
		*(u64*)((char*)f + sizeof(fptr)) += size;
	}
	csl_fs_flush(f);
}
ex csl_str csl_fs_read(fs f, u32 size) {
	csl_str s = csl_str_alloc(size);
	fread(s + 4, 1, size, *(fptr*)(f));
	return s;
}
ex void csl_fs_readmem(fs f, void* ptr, u32 size) {
	fread(ptr, 1, size, *(fptr*)(f));
}
ex fs csl_fs_create(csl_str str, i8 mode) {
	int a = 1;
	char b[4];
	if (mode > 2) {
		b[1] = '+';
		a++;
	}
	mode = mode % 3;
	const char* raw = "raw";
	b[0] = raw[mode];
	b[a] = 'b';
	b[a + 1] = '\0';
	fs f = csl_mem_alloc(sizeof(fptr) + sizeof(u64));
	csl_allocation cstr = csl_str_cstr(str);
	*(fptr*)(f) = fopen((char*)cstr, b);
	csl_mem_free(cstr);
	if (*(fptr*)f == 0) {
		csl_mem_free(f);
		return 0;
	}
	fseek(*(fptr*)(f), 0, 2);
	*(u64*)((char*)f + sizeof(fptr)) = csl_fs_cur(f);
	csl_fs_move(f, 0);
	return f;
}
ex void csl_fs_destroy(fs f) {
	fclose(*(fptr*)(f));
	csl_mem_free(f);
}