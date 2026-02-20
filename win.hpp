extern "C" {
    typedef void* HANDLE;
    typedef unsigned long DWORD;
    typedef int BOOL;
    typedef void* LPVOID;
    typedef DWORD(*LPTHREAD_START_ROUTINE)(LPVOID);

    HANDLE LoadLibraryA(const char* lpLibFileName);
    HANDLE GetModuleHandleA(const char* lpModuleName);
    //BOOL    FreeLibrary(HANDLE hModule);
    void* GetProcAddress(HANDLE hModule, const char* lpProcName);
}
using QPC = BOOL(__stdcall*)(long long*);

static HANDLE kernel32;
HANDLE(*pCreateThread)(void*, size_t, LPTHREAD_START_ROUTINE, void*, DWORD, DWORD*);
DWORD(*pWait)(HANDLE, DWORD);
BOOL(*pClose)(HANDLE);
QPC qpc;
QPC qpf;

struct ctx {
    void*(*func)(void*);
    void* data;
    csl_allocation ret;
};
static DWORD __stdcall csl_thread_handler(void* data) {
    ctx* ct = (ctx*)data;
    *(csl_allocation*)ct->ret = ct->func(ct->data);
    return 0;
}

struct thread {
    HANDLE handle = nullptr;
    DWORD id = 0;
    ctx ct;
    csl_allocation ret = nullptr;

    void start(void*(*func)(void*), void* data) {
        ct.func = func;
        ct.data = data;
        ct.ret = &ret;
        handle = pCreateThread(nullptr, 0, csl_thread_handler, &ct, 0, &id);
    }

    void* join() {
        if (handle) {
            pWait(handle, 0xFFFFFFFF);
            pClose(handle);
        }
        return ret;
    }
};
ex csl_thread csl_thread_branch(void* f, void* d) {
    thread* nt = new thread;
    nt->start((void*(*)(void*))f, d);
    return nt;
}
ex csl_allocation csl_thread_merge(csl_thread t) {
    thread* th = (thread*)t;
    th->join();
    csl_allocation ret = th->ret;
    delete th;
    return ret;
}

long long pf = 0;
void csl_platformspecific_init() {
    kernel32 = GetModuleHandleA("kernel32.dll");
    pCreateThread = (HANDLE(*)(void*, size_t, LPTHREAD_START_ROUTINE, void*, DWORD, DWORD*))GetProcAddress(kernel32, "CreateThread");
    pWait = (DWORD(*)(HANDLE, DWORD))GetProcAddress(kernel32, "WaitForSingleObject");
    pClose = (BOOL(*)(HANDLE))GetProcAddress(kernel32, "CloseHandle");
    qpc = (QPC)GetProcAddress(kernel32, "QueryPerformanceCounter");
    qpf = (QPC)GetProcAddress(kernel32, "QueryPerformanceFrequency");
    qpf(&pf);
}
csl_api csl_api_load(csl_str str) {
    csl_str cc = csl_str_concat(str, (csl_str)"\x04\x00\x00\x00.dll");
    csl_allocation s = csl_str_cstr(cc);
    csl_str_destroy(cc);
    void* r = LoadLibraryA((char*)s);
    csl_mem_free(s);
    return r;
}
void* csl_api_func(csl_api api, csl_str str) {
    csl_allocation s = csl_str_cstr(str);
    void* r = GetProcAddress(api, (char*)s);
    csl_mem_free(s);
    return r;
}
//void* csl_platformspecific_getptr() {
//    return GetModuleHandleA(NULL);
//}
ex f64 csl_time_get() {
    long long val = 0;
    qpc(&val);
    return (f64)val / (f64)pf;
}