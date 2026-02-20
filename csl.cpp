#include "defs.hpp"
#include "crt.hpp"

struct csl_global_type {
	u32 del;
	csl_vaddr jobs;
	csl_atomic fibers;
	csl_allocation fiberslist;
	u32 threads;
	csl_vaddr stasis_fs;
	csl_vaddr prgmglobals;
};
csl_global_type* csl_globals = 0;

#include "atomics.hpp"

// Time

ex inline void csl_time_pause() {
#if defined(_MSC_VER)
#if defined(_M_X64) || defined(_M_IX86)
	_mm_pause();
#elif defined(_M_ARM64)
	__yield();
#endif
#elif defined(__clang__) || defined(__GNUC__)
#if defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("pause");
#elif defined(__aarch64__)
	__asm__ volatile("yield");
#endif
#endif
}

// Mutex

ex void csl_mutex_acquire(csl_mutex* m) {
	while (csl_atomic_exchange(m, 1)) {
		while (csl_atomic_load(m)) {
			csl_time_pause();
		}
	}
}
ex void csl_mutex_release(csl_mutex* m) {
	csl_atomic_store(m, 0);
}

// Thread
#if defined(_WIN32)
extern "C" __declspec(dllimport) unsigned long __stdcall GetActiveProcessorCount(unsigned short);
#elif defined(__linux__)
extern "C" long sysconf(int);
#endif
ex unsigned csl_thread_count(void) {
#if defined(_WIN32)
	return (unsigned)GetActiveProcessorCount(0xFFFF);
#elif defined(__linux__)
	long n = sysconf(84);
	return (n > 0) ? (unsigned)n : 1;
#else
	return 1;
#endif
}

// Errors
csl_str err = 0;
ex csl_str csl_error_get() {
	return err;
}

#include "hwmem.hpp"
#include "vaddr.hpp"
#include "memory.hpp"
#include "math.hpp"
#include "strings.hpp"

// Platform specific
#if defined(_MSC_VER)
#include "win.hpp"
#endif

// Time (extended)
ex void csl_time_wait(f64 s) {
	f64 beg = csl_time_get();
	while (csl_time_get() - beg < s) {
		csl_time_pause();
	}
}

#include "filesystem.hpp"

// Debug
ex void csl_mem_dump(csl_allocation all, csl_str where) {
	u32 sz = csl_mem_size(all);
	if (sz > 0) {
		fs f = csl_fs_create(where, 2);
		csl_str s = (csl_str)csl_mem_alloc(sz + 4);
		csl_mem_cpy(s + 4, all, sz);
		*(u32*)s = sz;
		csl_fs_write(f, s);
		csl_str_destroy(s);
		csl_fs_destroy(f);
	}
}

#include "list.hpp"
#include "queue.hpp"
#include "jobs.hpp"
#include "fibers.hpp"

// Exit
ex void csl_program_exit(i32 code) {
	csl_fiber_shutdown();
	exit(code);
}
ex void csl_program_abort(i32 code) {
	exit(code);
}

// Initializer

ex void csl_program_init() {
	if (csl_globals == 0) {
		csl_globals = (csl_global_type*)csl_mem_zalloc(sizeof(csl_global_type));
	}
	//csl_vaddr_add(csl_platformspecific_getptr());
	csl_platformspecific_init();
	csl_fiber_init();
}

// Globals
ex void csl_globals_register(void* ptr) {
	csl_globals->prgmglobals = csl_vaddr_store(ptr);
}
ex void* csl_globals_get() {
	return csl_vaddr_get(csl_globals->prgmglobals);
}

#include "stasis.hpp"

void csl_test2(csl_allocation al) {
	csl_str_print((csl_str)al);
}

struct glob {
	csl_vaddr s;
	int i = 0;
};
#define g(v) ((glob*)csl_globals_get())->v
#define gptrg(v) csl_vaddr_get(((glob*)csl_globals_get())->v)

#define version 2

ex u32 csl_test() {
	if (!csl_stasis_resume(version, (csl_str)"\x04\x00\x00\x00test")) {
		void* g = csl_mem_alloc(sizeof(glob));
		csl_globals_register(g);
		g(s) = csl_vaddr_store(csl_str_create(str("hello, world!")));
		g(i) = 0;
	}
	g(i) += 1;
	csl_str a = csl_str_substr((csl_str)gptrg(s), 0, g(i));
	csl_str b = csl_str_concat(a, (csl_str)"\x01\x00\x00\x00\n");
	csl_str_destroy(a);
	csl_fiber_queue((ptr)csl_test2, b);
	csl_time_wait(.1);
	csl_str_destroy(b);
	csl_stasis_freeze(version, (csl_str)"\x04\x00\x00\x00test");
	return 0;
}