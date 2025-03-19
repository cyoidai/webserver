
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_THREADS 8

char *ROOT;
const char *BIND_IP = "127.0.0.1";
// const char *BIND_IP = "0.0.0.0";
const char *BIND_PORT = "8000";
int server_socket;
pthread_t threads[MAX_THREADS];
bool thread_active[MAX_THREADS] = {false};

struct thread_info
{
    int socket;
    int thread_index;
};

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

void server_get(char *response, size_t size, struct http_req *req)
{
    char fpath[128];
    strncpy(fpath, ROOT, sizeof(fpath));
    if (strcmp(req->path, "/") == 0)
        strcat(fpath, "/index.html");
    else
        strncat(fpath, req->path, sizeof(fpath) - strlen(fpath));

    strncpy(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", size);

    int fd = open(fpath, O_RDONLY);
    if (fd < 0)
    {
        strncpy(response, "HTTP/1.1 404 Not Found", size);
        return;
    }
    int bytes_in;
    if (bytes_in = read(fd, response, size) < 0)
    {
        perror("read");
        strncpy(response, "HTTP/1.1 404 Not Found", size);
        return;
    }
    sprintf(response, response, bytes_in);
    close(fd);

    // ensure the entire file was read
    // if (bytes_in = read(fd, body, sizeof(body)) != 0)
    // {
    //     fprintf(stderr, "File '%s' is too large", req->path);
    //     strncpy(response, "HTTP/1.1 404 Not Found", size);
    //     return;
    // }

}

void *accept_connection(void *_tinfo)
{
    struct thread_info *tinfo = (struct thread_info *)_tinfo;
    int socket = tinfo->socket;
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
    printf("%s\n", res);
    int bytes_out = send(socket, res, sizeof(res), MSG_NOSIGNAL);
    if (bytes_out < 0)
    {
        close(socket);
        return NULL;
    }
    close(socket);
    thread_active[tinfo->thread_index] = false;
    free(tinfo);
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
        for (int i = 0; i < MAX_THREADS; i++)
        {
            if (thread_active[i])
                continue;
            struct thread_info *tinfo = (struct thread_info *)malloc(sizeof(struct thread_info));
            tinfo->socket = client_socket;
            tinfo->thread_index = i;
            thread_active[i] = true;
            if (pthread_create(&threads[i], NULL, accept_connection, (void *)tinfo) == 0)
            {
                printf("Running on thread %d\n", i);
                break;
            }
            thread_active[i] = false;
            close(client_socket);
        }
            // if (pthread_tryjoin_np(threads[i], NULL) == 0)
            //     if (pthread_create(&threads[i], NULL, accept_connection, (void *)&client_socket) == 0)
            //         break;
    }
}

void stop(int signal)
{
    printf("Received %d, stopping...\n", signal);
    for(int i = 0; i < MAX_THREADS; i++)
    {
        void *response;
        pthread_join(threads[i], &response);
    }
    close(server_socket);
    printf("Finished\n");
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc == 2)
        ROOT = argv[1];
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

    struct sigaction sa;
    sa.sa_handler = &stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    server();
}
