// Strings

template<size_t N>
constexpr csl_str str(const char(&lit)[N]) {
	struct {
		u32 len;
		char data[N];
	} s { N - 1, lit };
	for (size_t i = 0; i < N; ++i)
		s.data[i] = lit[i];
	return (csl_str)&s;
}

//#define str(s) csl_str_fromcstr((csl_allocation)s)

csl_str csl_str_alloc(u32 size) {
	csl_str t = (char*)csl_mem_alloc(size + 4);
	*((u32*)t) = size;
	return t;
}
csl_str csl_str_cnew(csl_str text, u32 size, u32 copy = 0) {
	if (copy == 0) {
		copy = size;
	}
	csl_str t = csl_str_alloc(size);
	csl_mem_cpy(t + 4, (char*)text, copy);
	*(u32*)t = size;
	return t;
}
ex csl_str csl_str_create(csl_str text) {
	return csl_str_cnew(text + 4, *((u32*)text));
}
csl_mutex sout = 0;
ex void csl_str_print(csl_str ptr) {
	csl_mutex_acquire(&sout);
	for (int i = 0; i < *((u32*)ptr); i++) {
		putchar(((csl_str)ptr)[i + 4]);
	}
	csl_mutex_release(&sout);
}
ex csl_str csl_str_concat(csl_str ptr, csl_str ptr2) {
	u32 size = *((u32*)ptr) + *((u32*)ptr2);
	csl_str nstr = csl_str_cnew(ptr + 4, size, *((u32*)ptr));
	csl_mem_cpy(nstr + *((u32*)ptr) + 4, ptr2 + 4, *((u32*)ptr2));
	return nstr;
}
ex csl_str csl_str_substr(csl_str ptr, i32 pos, i32 count) {
	return csl_str_cnew(ptr + 4 + pos, count);
}
ex u32 csl_str_size(csl_str ptr) {
	return *((u32*)ptr);
}
ex csl_allocation csl_str_cstr(csl_str ptr) {
	u32 sz = csl_str_size(ptr);
	csl_allocation t = (char*)csl_mem_alloc(sz + 1);
	csl_mem_cpy(t, ptr + 4, sz);
	*(char*)((char*)t + sz) = '\0';
	return t;
}
ex csl_str csl_str_fromcstr(csl_allocation m) {
	int a = 0;
	while (true) {
		if (*(char*)((char*)m + a) == '\0') {
			break;
		}
		a++;
	}
	csl_str t = csl_str_alloc(a);
	*(u32*)((char*)t) = a;
	for (int i = 0; i < a; i++) {
		*(char*)((char*)t + i + 4) = *(char*)((char*)m + i);
	}
	return t;
}
ex csl_str csl_str_frommem(csl_allocation m, u32 size) {
	csl_str t = csl_str_alloc(size);
	*(u32*)((char*)t) = size;
	for (int i = 0; i < size; i++) {
		*(char*)((char*)t + i + 4) = *(char*)((char*)m + i);
	}
	return t;
}
ex csl_str csl_str_fromi32(i32 val) {
	int b = 1;
	int a = val;
	while (a >= 10) {
		a /= 10;
		b++;
	}
	csl_str str = csl_str_alloc(b);
	a = 1;
	while (b > 0) {
		a *= 10;
		b--;
	}
	const char* num = "0123456789";
	int c = 4;
	while (a > 1) {
		a /= 10;
		str[c] = num[((val / a) % 10)];
		c++;
	}
	return str;
}
ex void csl_str_destroy(csl_str ptr) {
	csl_mem_free(ptr);
}