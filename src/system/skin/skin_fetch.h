#ifndef SKIN_FETCH_H
#define SKIN_FETCH_H

#include <stdbool.h>
#include <stddef.h>

// Locates or downloads a skin by name.
// Returns true if the skin is found/downloaded and copied into the local config skins directory.
// 'out_path' will contain the absolute path to the skin PNG file.
bool fetch_skin(const char *skin_name, char *out_path, size_t out_path_size);

#endif // SKIN_FETCH_H
