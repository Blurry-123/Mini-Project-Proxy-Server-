#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "logger.h"

#define LOG_FILE "logs/proxy.log"

/*
 * Mutex prevents multiple threads from
 * writing to the log at the same time.
 */
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;


void log_request(
    const char *client_ip,
    const char *host,
    const char *method,
    const char *result,
    long milliseconds)
{
    FILE *file;

    time_t current_time;
    struct tm *time_info;

    current_time = time(NULL);
    time_info = localtime(&current_time);

    pthread_mutex_lock(&log_mutex);

    file = fopen(LOG_FILE, "a");

    if (file != NULL)
    {
        fprintf(
            file,

            "[%04d-%02d-%02d %02d:%02d:%02d] "
            "client=%s "
            "host=%s "
            "method=%s "
            "result=%s "
            "time=%ldms\n",

            time_info->tm_year + 1900,
            time_info->tm_mon + 1,
            time_info->tm_mday,

            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,

            client_ip,
            host,
            method,
            result,
            milliseconds
        );

        fclose(file);
    }

    pthread_mutex_unlock(&log_mutex);
}