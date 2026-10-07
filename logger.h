#ifndef LOGGER_H
#define LOGGER_H

/*
 * Write a request entry to the log.
 */
void log_request(
    const char *client_ip,
    const char *host,
    const char *method,
    const char *result,
    long milliseconds
);

#endif