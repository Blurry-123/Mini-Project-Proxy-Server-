#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>

#include "http.h"
#include "cache.h"
#include "access_control.h"
#include "logger.h"

#define PROXY_PORT 8080

#define BUFFER_SIZE 8192
#define CACHE_BUFFER_SIZE 1048576

#define BACKLOG 20


struct ClientInfo
{
    int client_fd;
    struct sockaddr_in client_address;
};



int send_all(
    int socket_fd,
    const char *data,
    size_t data_size)
{
    size_t total_sent = 0;

    while (total_sent < data_size)
    {
        ssize_t sent =
            send(
                socket_fd,
                data + total_sent,
                data_size - total_sent,
                0
            );

        if (sent <= 0)
        {
            return -1;
        }

        total_sent += sent;
    }

    return 0;
}

int connect_to_server(
    const char *host,
    int port)
{
    int server_fd;

    struct sockaddr_in server_address;

    struct hostent *server;

    server = gethostbyname(host);

    if (server == NULL)
    {
        return -1;
    }

    server_fd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (server_fd < 0)
    {
        return -1;
    }

    memset(
        &server_address,
        0,
        sizeof(server_address)
    );

    server_address.sin_family = AF_INET;

    server_address.sin_port =
        htons(port);

    memcpy(
        &server_address.sin_addr,
        server->h_addr,
        server->h_length
    );

   
    struct timeval timeout;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_SNDTIMEO,
        &timeout,
        sizeof(timeout)
    );

    if (
        connect(
            server_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0
    )
    {
        close(server_fd);

        return -1;
    }

    return server_fd;
}

void *handle_client(void *argument)
{
    struct ClientInfo *client;

    client =
        (struct ClientInfo *)argument;

    int client_fd =
        client->client_fd;

    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(
        AF_INET,
        &client->client_address.sin_addr,
        client_ip,
        sizeof(client_ip)
    );

    char request[BUFFER_SIZE];

    memset(
        request,
        0,
        sizeof(request)
    );

    ssize_t received =
        recv(
            client_fd,
            request,
            sizeof(request) - 1,
            0
        );

    if (received <= 0)
    {
        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }


    request[received] = '\0';
    struct HttpRequest req;

    int parse_result =
        parse_http_request(
            request,
            &req
        );

    if (parse_result == -1)
    {
        send_http_error(
            client_fd,
            400,
            "Bad Request"
        );

        log_request(
            client_ip,
            "-",
            "-",
            "BAD_REQUEST",
            0
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }

    if (parse_result == -2)
    {
        send_http_error(
            client_fd,
            501,
            "Not Implemented"
        );

        log_request(
            client_ip,
            req.host,
            req.method,
            "METHOD_NOT_SUPPORTED",
            0
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }


    struct timespec start_time;

    clock_gettime(
        CLOCK_MONOTONIC,
        &start_time
    );

    if (is_domain_blocked(req.host))
    {
        send_http_error(
            client_fd,
            403,
            "Forbidden"
        );

        log_request(
            client_ip,
            req.host,
            req.method,
            "BLOCKED",
            0
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }

    char cache_filename[512];

    create_cache_filename(
        req.host,
        req.path,
        cache_filename,
        sizeof(cache_filename)
    );

    if (cache_exists(cache_filename))
    {
        char *cached_data;

        cached_data =
            malloc(CACHE_BUFFER_SIZE);

        if (cached_data != NULL)
        {
            int cached_size =
                read_cache(
                    cache_filename,
                    cached_data,
                    CACHE_BUFFER_SIZE
                );

            if (cached_size > 0)
            {
                send_all(
                    client_fd,
                    cached_data,
                    cached_size
                );

                struct timespec end_time;

                clock_gettime(
                    CLOCK_MONOTONIC,
                    &end_time
                );

                long milliseconds =
                    (end_time.tv_sec -
                     start_time.tv_sec) * 1000
                    +
                    (end_time.tv_nsec -
                     start_time.tv_nsec) / 1000000;

                log_request(
                    client_ip,
                    req.host,
                    req.method,
                    "CACHE_HIT",
                    milliseconds
                );

                free(cached_data);

                close(client_fd);

                free(client);

                pthread_exit(NULL);
            }

            free(cached_data);
        }
    }

    int server_fd =
        connect_to_server(
            req.host,
            req.port
        );


    if (server_fd < 0)
    {
        send_http_error(
            client_fd,
            502,
            "Bad Gateway"
        );

        log_request(
            client_ip,
            req.host,
            req.method,
            "DESTINATION_ERROR",
            0
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }

    char outgoing_request[BUFFER_SIZE];

    snprintf(
        outgoing_request,
        sizeof(outgoing_request),

        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",

        req.path,
        req.host
    );

    if (
        send_all(
            server_fd,
            outgoing_request,
            strlen(outgoing_request)
        ) < 0
    )
    {
        close(server_fd);

        send_http_error(
            client_fd,
            502,
            "Bad Gateway"
        );

        log_request(
            client_ip,
            req.host,
            req.method,
            "SEND_ERROR",
            0
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }
    char *response_buffer;

    response_buffer =
        malloc(CACHE_BUFFER_SIZE);

    if (response_buffer == NULL)
    {
        close(server_fd);

        send_http_error(
            client_fd,
            500,
            "Internal Server Error"
        );

        close(client_fd);

        free(client);

        pthread_exit(NULL);
    }


    size_t total_received = 0;

    int receive_error = 0;


    while (
        total_received <
        CACHE_BUFFER_SIZE
    )
    {
        ssize_t bytes =
            recv(
                server_fd,
                response_buffer +
                    total_received,
                CACHE_BUFFER_SIZE -
                    total_received,
                0
            );

        if (bytes == 0)
        {
            break;
        }

        if (bytes < 0)
        {
            receive_error = 1;

            break;
        }

        total_received += bytes;

        if (
            send_all(
                client_fd,
                response_buffer +
                    total_received -
                    bytes,
                bytes
            ) < 0
        )
        {
            break;
        }
    }


    close(server_fd);

    if (
        !receive_error &&
        total_received > 0 &&
        strncmp(
            response_buffer,
            "HTTP/1.0 200",
            12
        ) == 0
    )
    {
        write_cache(
            cache_filename,
            response_buffer,
            total_received
        );
    }

    struct timespec end_time;

    clock_gettime(
        CLOCK_MONOTONIC,
        &end_time
    );

    long milliseconds =
        (end_time.tv_sec -
         start_time.tv_sec) * 1000
        +
        (end_time.tv_nsec -
         start_time.tv_nsec) / 1000000;


    if (receive_error)
    {
        log_request(
            client_ip,
            req.host,
            req.method,
            "RECEIVE_ERROR",
            milliseconds
        );
    }
    else
    {
        log_request(
            client_ip,
            req.host,
            req.method,
            "CACHE_MISS_FORWARD",
            milliseconds
        );
    }


    free(response_buffer);

    close(client_fd);

    free(client);

    pthread_exit(NULL);
}

int main()
{
    int server_fd;

    struct sockaddr_in server_address;

    signal(
        SIGPIPE,
        SIG_IGN
    );
    mkdir(
        "cache",
        0755
    );

    mkdir(
        "logs",
        0755
    );

    load_blocked_domains(
        "blocked_domains.txt"
    );
    server_fd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (server_fd < 0)
    {
        perror("socket");

        return 1;
    }
    int option = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    );
    memset(
        &server_address,
        0,
        sizeof(server_address)
    );

    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(PROXY_PORT);

    if (
        bind(
            server_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0
    )
    {
        perror("bind");

        close(server_fd);

        return 1;
    }

    if (
        listen(
            server_fd,
            BACKLOG
        ) < 0
    )
    {
        perror("listen");

        close(server_fd);

        return 1;
    }


    printf(
        "\n=================================\n"
        "      PROXY SERVER STARTED\n"
        "=================================\n"
        "Port       : %d\n"
        "Cache TTL  : %d seconds\n"
        "Protocol   : HTTP\n"
        "=================================\n\n",

        PROXY_PORT,
        CACHE_TTL
    );

    while (1)
    {
        struct ClientInfo *client;

        client =
            malloc(
                sizeof(struct ClientInfo)
            );

        if (client == NULL)
        {
            continue;
        }


        socklen_t address_length =
            sizeof(client->client_address);


        client->client_fd =
            accept(
                server_fd,
                (struct sockaddr *)
                    &client->client_address,
                &address_length
            );


        if (client->client_fd < 0)
        {
            free(client);

            continue;
        }

        pthread_t thread;

        if (
            pthread_create(
                &thread,
                NULL,
                handle_client,
                client
            ) != 0
        )
        {
            close(client->client_fd);

            free(client);

            continue;
        }

        pthread_detach(thread);
    }


    close(server_fd);

    return 0;
}
