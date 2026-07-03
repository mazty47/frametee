#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Opens a file with UTF-8 path support.
 * 
 * On Windows, this converts the UTF-8 path and mode to UTF-16 and uses _wfopen.
 * On other platforms, it calls fopen directly.
 */
FILE *fs_open(const char *path, const char *mode);

/**
 * @brief Gets the config directory path.
 * 
 * Returns true if successful and out_path contains the path.
 */
bool fs_get_config_dir(char *out_path, size_t size);

#endif // FS_H
