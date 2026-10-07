#ifndef LOGGER_H
#define LOGGER_H

void log_request(
    const char *client_ip,
    const char *host,
    const char *method,
    const char *result,
    long milliseconds
);

#endif
