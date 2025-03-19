
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"
#include "wwf.h"

#define MAX_HTTP_REQUEST_SIZE 2048
#define MAX_HTTP_RESPONSE_SIZE 16384
#define MAX_HTTP_RESPONSE_BODY_SIZE 15872

char *ROOT;
// const char *BIND_IP = "127.0.0.1";
const char *BIND_IP = "0.0.0.0";
const char *BIND_PORT = "8000";
const char *HTTP_DEFAULT_404_RESPONSE =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: 137\r\n\r\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "  <body>\n"
    "    <h1>404 Page not found</h1>\n"
    "    <p>The page you are looking for doesn't exist.</p>\n"
    "  </body>\n"
    "</html>";

struct http_req
{
    char *method;
    char *path;
    char *query;
    char *version;
};

void http_req_parse(struct http_req *req, char *str)
{
    char *line = strtok_r(str, "\r\n", &str);
    req->method = strtok_r(line, " ", &line);
    char *path = strtok_r(line, " ", &line);
    req->path = strtok_r(path, "?", &path);
    req->query = path;
    strtok_r(line, "/", &line);
    req->version = line;
}

/**
 * @brief Handle an HTTP GET request
 * @param res  Pointer to store the raw HTTP response in.
 * @param size Size of response.
 * @param req  Client's HTTP request.
 */
void server_get(char *res, size_t size, struct http_req *req)
{
    char body[MAX_HTTP_RESPONSE_BODY_SIZE] = {0};
    // overrides
    if (strcmp(req->path, "/words") == 0)
    {
        if (strlen(req->query) == 0)
            gameNew();
        else
        {
            char *kv_pair;
            while (kv_pair = strtok_r(req->query, "&", &req->query))
                if (strcmp(strtok_r(kv_pair, "=", &kv_pair), "guess") == 0)
                {
                    strupper(kv_pair);
                    acceptInput(kv_pair);
                    break;
                }
        }
        displayWorldHTML(body, sizeof(body));
    }
    // default behavior, look for file
    else
    {
        char fpath[256];
        strncpy(fpath, ROOT, sizeof(fpath) - 1);
        strncat(fpath, req->path, sizeof(fpath) - strlen(fpath));
        int fd = open(fpath, O_RDONLY);
        if (fd < 0)
        {
            perror("open");
            strncpy(res, HTTP_DEFAULT_404_RESPONSE, size - 1);
            return;
        }
        int bytes_in;
        if (bytes_in = read(fd, body, sizeof(body) - 1) < 0)
        {
            perror("read");
            strncpy(res, HTTP_DEFAULT_404_RESPONSE, size - 1);
            return;
        }
        close(fd);
    }
    snprintf(res, size,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s", strlen(body), body);
}

void *handle_connection(void *client_socket)
{
    int socket = *(int *)client_socket;
    char req_raw[MAX_HTTP_REQUEST_SIZE] = {0};
    int bytes_in = recv(socket, req_raw, sizeof(req_raw) - 1, 0);
    if (bytes_in < 0)
    {
        perror("recv");
        close(socket);
        return NULL;
    }
    struct http_req req;
    http_req_parse(&req, req_raw);
    printf("HTTP/%s %s %s %s\n", req.version, req.method, req.path, req.query);
    char res[MAX_HTTP_RESPONSE_SIZE];
    if (strcmp(req.method, "GET") == 0)
        server_get(res, sizeof(res), &req);
    else
    {
        fprintf(stderr, "Unknown HTTP method: '%s'\n", req.method);
        close(socket);
        return NULL;
    }
    if (strlen(res) == MAX_HTTP_RESPONSE_SIZE - 1)
    {
        fprintf(stderr, "Response too large\n");
        close(socket);
        return NULL;
    }
    int bytes_out = send(socket, res, sizeof(res), MSG_NOSIGNAL);
    if (bytes_out < 0)
    {
        perror("send");
        close(socket);
        return NULL;
    }
    close(socket);
    return NULL;
}

int server()
{
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(BIND_IP, BIND_PORT, &hints, &res);

    struct addrinfo *ai;
    int server_socket;
    for (ai = res; ai != NULL; ai = ai->ai_next)
    {
        server_socket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (server_socket == -1)
            continue;
        if (bind(server_socket, ai->ai_addr, ai->ai_addrlen) == 1)
        {
            perror("bind");
            exit(1);
        }
        break;
    }
    freeaddrinfo(res);
    if (listen(server_socket, 16) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("Listening on %s tcp/%s\n", BIND_IP, BIND_PORT);
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int client_socket;
    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addrlen);
        if (client_socket < 0)
            continue;
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_connection, (void *)&client_socket) != 0)
            close(client_socket);
    }
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        ROOT = argv[1];
        // remove possible trailing '/' from directory
        int last = strlen(ROOT) - 1;
        if (ROOT[last] == '/')
            ROOT[last] = '\0';
    }
    else
    {
        fprintf(stderr, "Illegal number of arguments\n");
        printf("Usage: %s ROOT\nWhere ROOT is the webserver's root path.\n", argv[0]);
        return 1;
    }

    // ensure the root path for the webserver exists
    struct stat s;
    if (lstat(ROOT, &s) == -1)
    {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(s.st_mode))
    {
        fprintf(stderr, "Root path '%s' must be a directory\n", ROOT);
        return 1;
    }

    if (initialize() != 0)
    {
        fprintf(stderr, "Failed to initialize game\n");
        return 1;
    }

    server();
}
