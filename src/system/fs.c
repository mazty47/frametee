#include "fs.h"

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>

FILE *fs_open(const char *path, const char *mode) {
    wchar_t wpath[1024];
    wchar_t wmode[16];
    
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024) <= 0) {
        return NULL;
    }
    
    if (MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16) <= 0) {
        return NULL;
    }
    
    return _wfopen(wpath, wmode);
}
#else
FILE *fs_open(const char *path, const char *mode) {
    return fopen(path, mode);
}
#endif
