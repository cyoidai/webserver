
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

char *ROOT;
// const char *BIND_IP = "127.0.0.1";
const char *BIND_IP = "0.0.0.0";
const char *BIND_PORT = "8000";
const char *HTTP_DEFAULT_404_RESPONSE =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Length: 142\r\n\r\n"
    "<!DOCTYPE html>"
    "<html>"
    "    <body>"
    "        <h1>404 Page not found</h1>"
    "        <p>The page you are looking for doesn't exist.</p>"
    "    </body>"
    "</html>";
int server_socket;

struct http_req
{
    char *version;
    char *path;
    char *method;
};

void http_req_parse(struct http_req *req, char *str)
{
    char *line = strtok_r(str, "\r\n", &str);
    char *item;
    req->method = strtok_r(line, " ", &line);
    req->path = strtok_r(line, " ", &line);
    req->version = strtok_r(line, " ", &line);
}

/**
 * @brief Handle an HTTP GET request
 * @param res  Pointer to store the raw HTTP response in.
 * @param size Size of response.
 * @param req  Client's HTTP request.
 */
void server_get(char *res, size_t size, struct http_req *req)
{
    char fpath[128];
    strncpy(fpath, ROOT, sizeof(fpath));
    strncat(fpath, req->path, sizeof(fpath) - strlen(fpath));
    struct stat s;
    if (lstat(fpath, &s) == -1)
    {
        strncpy(res, HTTP_DEFAULT_404_RESPONSE, size);
        return;
    }
    int content_length = s.st_size;
    int fd = open(fpath, O_RDONLY);
    if (fd < 0)
    {
        strncpy(res, HTTP_DEFAULT_404_RESPONSE, size);
        return;
    }
    char body[2048] = {0};
    int bytes_in;
    if (bytes_in = read(fd, body, sizeof(body) - 1) < 0)
    {
        strncpy(res, HTTP_DEFAULT_404_RESPONSE, size);
        return;
    }
    close(fd);
    snprintf(res, size,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        content_length, body);
}

void *accept_connection(void *client_socket)
{
    int socket = *(int *)client_socket;
    char req_raw[1024];
    int bytes_in = recv(socket, req_raw, sizeof(req_raw), 0);
    if (bytes_in < 0)
    {
        close(socket);
        return NULL;
    }
    struct http_req req;
    http_req_parse(&req, req_raw);
    printf("%s %s %s\n", req.method, req.path, req.version);
    char res[2048];
    if (strncmp(req.method, "GET", 3) == 0)
        server_get(res, sizeof(res), &req);
    else
    {
        close(socket);
        return NULL;
    }
    int bytes_out = send(socket, res, sizeof(res), MSG_NOSIGNAL);
    if (bytes_out < 0)
    {
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
        if (pthread_create(&thread, NULL, accept_connection, (void *)&client_socket) != 0)
            close(client_socket);
    }
}

int main(int argc, char **argv)
{
    if (argc == 2) {
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

    server();
}
