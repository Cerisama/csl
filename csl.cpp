#define ex extern "C"

using fs = void*;

//#define io
#ifdef io
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using fptr = FILE*;
#else
//ex unsigned long __rdtsc();
ex void putchar(char);
using fptr = void*;
ex fptr fopen(const char* filename, const char* mode);
ex int fclose(fptr stream);
ex unsigned long fread(void* buffer, unsigned long size, unsigned long count, fptr stream);
ex unsigned long fwrite(const void* buffer, unsigned long size, unsigned long count, fptr stream);
ex int fseek(fptr stream, long offset, int origin);
ex long ftell(fptr stream);
ex void printf(char* text);
#endif

ex void* memcpy(void* dst, const void* src, unsigned long long size);
ex void* malloc(unsigned long long size);
ex void free(void* ptr);

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long;
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long;
using csl_mutex = volatile int;
using csl_str = char*;
using csl_mempageptr = void*;
using csl_mempage = void*;
using csl_allocation = void*;

// Atomic

#if defined(_MSC_VER)
#include <intrin.h>
#endif

inline int csl_atomic_exchange_i32(volatile int* ptr, int v) {
#if defined(_MSC_VER)
	return _InterlockedExchange((volatile long*)ptr, v);
#elif defined(__clang__) || defined(__GNUC__)
	return __sync_lock_test_and_set(ptr, v);
#else
#error Unsupported compiler
#endif
}

inline int csl_atomic_load_i32(volatile int* ptr) {
#if defined(_MSC_VER)
	return *ptr; // x86 is already acquire-safe
#elif defined(__clang__) || defined(__GNUC__)
	return __sync_fetch_and_add(ptr, 0);
#endif
}

inline void csl_atomic_store_i32(volatile int* ptr, int v) {
#if defined(_MSC_VER)
	*ptr = v;
	_ReadWriteBarrier();
#elif defined(__clang__) || defined(__GNUC__)
	__sync_lock_release(ptr);
#endif
}

// Time

ex void csl_time_pause() {
#if defined(_MSC_VER)
	_mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("pause");
#endif
}

// Mutex

ex void csl_mutex_acquire(csl_mutex* m) {
	while (csl_atomic_exchange_i32(m, 1)) {
		while (csl_atomic_load_i32(m)) {
			csl_time_pause();
		}
	}
}
ex void csl_mutex_release(csl_mutex* m) {
	csl_atomic_store_i32(m, 0);
}

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
	csl_mempageptr nmp = malloc(4 + (size * sizeof(void*)));
	*((u32*)nmp) = size;
	if (mp != nullptr) {
		memcpy((char*)nmp + 4, (char*)mp + 4, (size - 1) * sizeof(void*));
	}
	*(void**)((char*)nmp + 4 + ((size - 1) * sizeof(void*))) = pmp;
	free(mp);
	mp = nmp;
}
void csl_mem_deletepage(u32 n) {
	u32 size = *((u32*)mp) - 1;
	csl_mempageptr nmp = malloc(4 + (size * sizeof(void*)));
	*((u32*)nmp) = size;
	if (n != 0) {
		memcpy((char*)nmp + 4, (char*)mp + 4, n * sizeof(void*));
	}
	if (n < size) {
		memcpy((char*)nmp + 4 + (sizeof(void*) * n), (char*)mp + 4 + (sizeof(void*) * (n + 1)), (size - n - 1) * sizeof(void*));
	}
	free(mp);
	mp = nmp;
}
ex void csl_mem_set(void* ptr, u32 size, u8 val) {
	u32 bval = (val >> 24) || (val >> 16) || (val >> 8) || val;
	for (int i = 0; i < size/4; i++) {
		*((u32*)((char*)ptr + (i * 4))) = bval;
	}
	for (int i = 0; i < size % 4; i++) {
		*((u8*)((char*)ptr + ((size / 4) * 4)) + i) = val;
	}
}
ex void csl_mem_zero(void* ptr, u32 size) {
	csl_mem_set(ptr, size, 0x00);
}
csl_mempage csl_mem_createpage(u32 blocksize, u32 blocks) {
	u64 pagesize = 12 + (blocks/8) + (blocksize * blocks);
	csl_mempage tmp = malloc(pagesize);
	*((u32*)tmp) = blocksize;
	*((u32*)((char*)tmp + 4)) = blocks;
	return tmp;
}
int asize = 0;
csl_mempage csl_mem_getpage(u32 n) {
	bool cnew = mp == nullptr;
	if (!cnew) {
		cnew = n > *((u32*)mp);
	}
	if (cnew) {
		csl_mem_pushpage(csl_mem_createpage(32, 16384));
	}
	return *((csl_mempage*)((char*)mp + (sizeof(void*) * n) + 4));
}
ex csl_allocation csl_mem_alloc(u32 size) {
	if (size > 32768) { // 32kb
		size += 4;
		csl_allocation tmp = malloc(size);
		*(u32*)(tmp) = 0;
		return (char*)tmp + 4;
	}
	else {
		csl_mutex_acquire(&mm);
		size += sizeof(void*) + 4;
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
			for (u32 i = sbl / 8; i < blocks / 8; i++) {
				char val = *((char*)((char*)pg + 12 + i));
				for (int a = 0; a < 8; a++) {
					bool freeblock = ((val >> a) & 1) == 0;
					if (freeblock) {
						ui++;
						if (ui == reqb) {
							ui = (i * 8) + a;
							found = true;
							break;
						}
					}
					else {
						ui = 0;
					}
				}
				if (found) {
					break;
				}
			}
			if (found) {
				lastpage = v;
				lastblock = ui + reqb;
				*(u32*)((char*)pg + 8) += reqb;
				for (u32 a = ui; a < ui + reqb; a++) {
					*((char*)((char*)pg + 12 + (a / 8))) |= (1 << (a % 8));
				}
				csl_allocation al = ((char*)pg + sizeof(void*) + 4 + 12 + (blocks / 8) + (ui * blocksize));
				*(char**)((char*)al - 4 - sizeof(void*)) = (char*)pg;
				*(u32*)((u32*)al - 4) = reqb;
				csl_mutex_release(&mm);
				return al;
			}
			v++;
		}
	}
}
ex csl_allocation csl_mem_zalloc(u32 size) {
	csl_allocation ma = csl_mem_alloc(size);
	csl_mem_zero(ma, size);
	return ma;
}
ex void csl_mem_free(csl_allocation ptr) {
	csl_mutex_acquire(&mm);
	u32 reqb = *(u32*)((u32*)ptr - 4);
	if (reqb == 0) {
		free((char*)ptr-4);
		return;
	}
	csl_mempage pg = *(char**)((char*)ptr - sizeof(void*) - 4);
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
		csl_mem_deletepage(i);
		lastpage = 0;
		lastblock = 0;
		csl_mutex_release(&mm);
		return;
	}
	u32 ui = ((char*)ptr - pg - (blocks / 8) - 8)/blocksize;
	if (i < lastpage || (ui + reqb < lastblock && i == lastpage)) {
		lastpage = i;
	}
	lastblock = ui + reqb;
	for (u32 a = ui; a < ui + reqb; a++) {
		*((char*)((char*)pg + 12 + (a / 8))) &= ~(1 << (a % 8));
	}
	csl_mutex_release(&mm);
}
ex void csl_mem_cpy(csl_allocation dst, csl_allocation src, u32 size) {
	memcpy(dst, src, size);
}

// Math

u64 state[2];
ex void csl_math_setseed(u64 seed) {
	auto splitmix = [](u64& x) {
		u64 z = (x += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return z ^ (z >> 31);
	};

	u64 x = seed;
	state[0] = splitmix(x);
	state[1] = splitmix(x);
}
ex u64 csl_math_randomseed() {
	return __rdtsc();
}
ex i32 csl_math_random(i32 lower, i32 upper) {
	u64 s1 = state[0];
	u64 s0 = state[1];

	state[0] = s0;
	s1 ^= s1 << 23;
	s1 ^= s1 >> 17;
	s1 ^= s0;
	s1 ^= s0 >> 26;
	state[1] = s1;

	u64 v = s1 + s0;
	i32 size = upper - lower + 1;
	return (v % size) + lower;
}

// Strings

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
	memcpy(t + 4, (char*)text, size);
	*(u32*)t = size;
	return t;
}
ex csl_str csl_str_create(csl_str text) {
	return csl_str_cnew(text + 4, *((u32*)text));
}
ex void csl_str_print(csl_str ptr) {
	for (int i = 0; i < *((u32*)ptr); i++) {
		putchar(((csl_str)ptr)[i + 4]);
	}
}
ex csl_str csl_str_concat(csl_str ptr, csl_str ptr2) {
	u32 size = *((u32*)ptr) + *((u32*)ptr2);
	csl_str nstr = csl_str_cnew(ptr + 4, size, *((u32*)ptr));
	memcpy(nstr + *((u32*)ptr) + 4, ptr2 + 4, *((u32*)ptr2));
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
	for (int i = 0; i < a; i++) {
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

// Filesystem

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
	if (sz == *(u64*)((char*)f + sizeof(fptr))) {
		*(u64*)((char*)f + sizeof(fptr)) += sz;
	}
}
ex csl_str csl_fs_read(fs f, u32 size) {
	csl_str s = csl_str_alloc(size);
	fread(s + 4, 1, size, *(fptr*)(f));
	return s;
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
	*(fptr*)(f) = fopen((char*)csl_str_cstr(str), b);
	fseek(*(fptr*)(f), 0, 2);
	*(u64*)((char*)f + sizeof(fptr)) = csl_fs_cur(f);
	csl_fs_move(f, 0);
	return f;
}
ex void csl_fs_destroy(fs f) {
	fclose(*(fptr*)(f));
	csl_mem_free(f);
}