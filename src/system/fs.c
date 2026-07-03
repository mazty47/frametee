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

#include <stdlib.h>

bool fs_get_config_dir(char *out_path, size_t size) {
#ifdef _WIN32
    const char *config_home = getenv("APPDATA");
    if (!config_home) config_home = getenv("USERPROFILE");
    if (config_home) {
        snprintf(out_path, size, "%s\\frametee", config_home);
        return true;
    }
#else
    const char *config_home = getenv("XDG_CONFIG_HOME");
    if (config_home) {
        snprintf(out_path, size, "%s/frametee", config_home);
        return true;
    } else {
        config_home = getenv("HOME");
        if (config_home) {
            snprintf(out_path, size, "%s/.config/frametee", config_home);
            return true;
        }
    }
#endif
    return false;
}
