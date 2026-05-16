#ifndef FS_H
#define FS_H

#include <stdio.h>

/**
 * @brief Opens a file with UTF-8 path support.
 * 
 * On Windows, this converts the UTF-8 path and mode to UTF-16 and uses _wfopen.
 * On other platforms, it calls fopen directly.
 */
FILE *fs_open(const char *path, const char *mode);

#endif // FS_H
