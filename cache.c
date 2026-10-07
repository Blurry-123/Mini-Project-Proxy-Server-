#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "cache.h"
void create_cache_filename(
    const char *host,
    const char *path,
    char *filename,
    size_t filename_size)
{
    snprintf(
        filename,
        filename_size,
        "%s/%s_%lu.cache",
        CACHE_DIRECTORY,
        host,
        (unsigned long)strlen(path)
    );
}

int cache_exists(const char *filename)
{
    struct stat file_info;

    if (stat(filename, &file_info) != 0)
    {
        return 0;
    }

    time_t current_time = time(NULL);

   
    if (difftime(current_time, file_info.st_mtime) > CACHE_TTL)
    {
        return 0;
    }

    return 1;
}

int read_cache(
    const char *filename,
    char *buffer,
    size_t buffer_size)
{
    FILE *file;

    file = fopen(filename, "rb");

    if (file == NULL)
    {
        return -1;
    }

    size_t bytes_read =
        fread(buffer, 1, buffer_size, file);

    fclose(file);

    return (int)bytes_read;
}

int write_cache(
    const char *filename,
    const char *data,
    size_t data_size)
{
    FILE *file;

    file = fopen(filename, "wb");

    if (file == NULL)
    {
        return -1;
    }

    fwrite(data, 1, data_size, file);

    fclose(file);

    return 0;
}
