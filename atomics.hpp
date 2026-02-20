// Atomic

#if defined(_MSC_VER)
#include <intrin.h>
#endif

ex inline int csl_atomic_exchange(csl_atomic* ptr, int v) {
#if defined(_MSC_VER)
	return _InterlockedExchange((volatile long*)ptr, v);
#elif defined(__clang__) || defined(__GNUC__)
	return __atomic_exchange_n(ptr, v, __ATOMIC_ACQ_REL);
#else
#error Unsupported compiler
#endif
}

ex inline int csl_atomic_load(const csl_atomic* ptr) {
#if defined(_MSC_VER)
	// MSVC: use interlocked OR 0 to enforce acquire
	return _InterlockedOr((volatile long*)ptr, 0);
#elif defined(__clang__) || defined(__GNUC__)
	return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
#else
#error Unsupported compiler
#endif
}

ex inline void csl_atomic_store(csl_atomic* ptr, int v) {
#if defined(_MSC_VER)
	// MSVC has no release-store primitive
	_InterlockedExchange((volatile long*)ptr, v);
#elif defined(__clang__) || defined(__GNUC__)
	__atomic_store_n(ptr, v, __ATOMIC_RELEASE);
#else
#error Unsupported compiler
#endif
}

ex inline void csl_atomic_inc(csl_atomic* ptr) {
#if defined(_MSC_VER)
	_InterlockedIncrement((volatile long*)ptr);
#elif defined(__GNUC__) || defined(__clang__)
	__atomic_add_fetch(ptr, 1, __ATOMIC_SEQ_CST);
#endif
}

ex inline void csl_atomic_dec(csl_atomic* ptr) {
#if defined(_MSC_VER)
	_InterlockedDecrement((volatile long*)ptr);
#elif defined(__GNUC__) || defined(__clang__)
	__atomic_sub_fetch(ptr, 1, __ATOMIC_SEQ_CST);
#endif
}