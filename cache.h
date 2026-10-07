#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

#define CACHE_DIRECTORY "cache"
#define CACHE_TTL 60

/*
 * Generate a filename for a cached URL.
 */
void create_cache_filename(
    const char *host,
    const char *path,
    char *filename,
    size_t filename_size
);

/*
 * Check whether a cached response exists
 * and is still valid.
 */
int cache_exists(const char *filename);

/*
 * Read cached data.
 */
int read_cache(
    const char *filename,
    char *buffer,
    size_t buffer_size
);

/*
 * Save response to cache.
 */
int write_cache(
    const char *filename,
    const char *data,
    size_t data_size
);

#endif