#define io
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
ex int fflush(fptr);
ex long ftell(fptr stream);
ex void printf(char* text);
#endif

#define usecrt false

ex void* memset(void* dest, int value, size_t count);
ex void* memcpy(void* dst, const void* src, unsigned long long size);
ex void* malloc(unsigned long long size);
ex void free(void* ptr);
ex void exit(int val);