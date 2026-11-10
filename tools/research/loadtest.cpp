#include <windows.h>
#include <stdio.h>
int main(int argc, char** argv) { setvbuf(stdout, 0, _IONBF, 0);
    HMODULE h = LoadLibraryA(argv[1]);
    printf("LoadLibrary=%p err=%lu\n", h, GetLastError());
    if (h) { FARPROC p = GetProcAddress(h, "Direct3DCreate9"); printf("proc=%p\n", p);
      void* d3d = p ? ((void*(WINAPI*)(UINT))p)(32) : 0; printf("d3d=%p\n", d3d); }
    return 0;
}
