#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "http.h"

/*
 * Parse the HTTP request received from the client.
 */
int parse_http_request(char *request, struct HttpRequest *req)
{
    char method[16];
    char url[2048];
    char version[16];

    char *host_header;

    /*
     * Read the first line.
     *
     * Example:
     *
     * GET http://example.com/index.html HTTP/1.0
     */
    if (sscanf(request, "%15s %2047s %15s",
               method, url, version) != 3)
    {
        return -1;
    }

    strcpy(req->method, method);

    /*
     * For this project we support GET.
     */
    if (strcmp(req->method, "GET") != 0)
    {
        return -2;
    }

    /*
     * Find the Host header.
     */
    host_header = strstr(request, "\nHost:");

    if (host_header == NULL)
    {
        host_header = strstr(request, "\nhost:");
    }

    /*
     * Parse URL.
     *
     * Example:
     *
     * http://example.com/index.html
     */
    if (strncmp(url, "http://", 7) == 0)
    {
        char *host_start = url + 7;
        char *path_start = strchr(host_start, '/');

        if (path_start != NULL)
        {
            strcpy(req->path, path_start);
            *path_start = '\0';
        }
        else
        {
            strcpy(req->path, "/");
        }

        strcpy(req->host, host_start);
    }
    else
    {
        /*
         * If the proxy receives an origin-form request,
         * use the Host header.
         */
        strcpy(req->path, url);

        if (req->path[0] == '\0')
        {
            strcpy(req->path, "/");
        }

        if (host_header != NULL)
        {
            sscanf(host_header, "\nHost: %255s", req->host);
        }
        else
        {
            return -1;
        }
    }

    /*
     * Default HTTP port.
     */
    req->port = 80;

    /*
     * Check if the host contains a port.
     *
     * Example:
     *
     * localhost:9000
     */
    char *colon = strchr(req->host, ':');

    if (colon != NULL)
    {
        *colon = '\0';
        req->port = atoi(colon + 1);
    }

    return 0;
}


/*
 * Send an HTTP error response to the client.
 */
void send_http_error(int client_fd, int status_code, const char *message)
{
    char response[1024];

    int length = snprintf(
        response,
        sizeof(response),

        "HTTP/1.0 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s\n",

        status_code,
        message,
        (long)(strlen(message) + 1),
        message
    );

    send(client_fd, response, length, 0);
}