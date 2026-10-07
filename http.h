#ifndef HTTP_H
#define HTTP_H

#define MAX_REQUEST_SIZE 8192
#define MAX_HOST_SIZE 256
#define MAX_PATH_SIZE 2048
#define MAX_METHOD_SIZE 16

struct HttpRequest
{
    char method[MAX_METHOD_SIZE];
    char host[MAX_HOST_SIZE];
    int port;
    char path[MAX_PATH_SIZE];
};

/* Parse an HTTP request */
int parse_http_request(char *request, struct HttpRequest *req);

/* Send an HTTP error response */
void send_http_error(int client_fd, int status_code, const char *message);

#endif