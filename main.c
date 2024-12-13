#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct file_data {
    char *buffer;
    size_t size;
};

struct file_data read_file(const char *filename) {
    struct file_data file_info = {NULL, 0};

    if (strcmp(filename, "/") == 0) {
        filename = "index.html";
    } else {
        filename = &filename[1];
    }

    FILE *file;
    file = fopen(filename, "rb");

    if (!file) {
        perror("Error opening file");
        return file_info;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        perror("Malloc failed");
        fclose(file);
        return file_info;
    }

    size_t read_size = fread(buffer, sizeof(char), file_size, file);

    if (read_size != file_size) {
        printf("%s\n", filename);
        perror("Failed to open entire file");
        free(buffer);
        fclose(file);
        return file_info;
    }

    buffer[read_size] = '\0';

    file_info.buffer = buffer;
    file_info.size = read_size;

    fclose(file);
    return file_info;
}

int create_socket(const char *domain, const int port) {
    int server_fd;
    struct sockaddr_in server_addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(domain);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        perror("Failed Listen");
        return -1;
    }

    printf("Server listening on %s:%d\n", domain, port);

    return server_fd;
}

void handle_requests(int server_fd) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    for (;;) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        char buffer[1024];
        recv(client_fd, buffer, sizeof(buffer), 0);
        char *buffer_copy = strdup(buffer);

        char *request_file = strtok(strchr(strtok(buffer, "\n"), '/'), " ");
        char *request_accept = strtok(buffer_copy, "\n");
        request_accept = strtok(NULL, "\n");
        request_accept = strtok(NULL, "\n");
        request_accept = strtok(NULL, "\n");
        request_accept = strtok(&strtok(request_accept, ",")[0], " ");
        request_accept = strtok(NULL, " ");

        if (strcmp(request_accept, "image/avif") == 0) {
            request_accept = "image/png";
        }

        struct file_data file_data = read_file(request_file);
        if (file_data.buffer == NULL || file_data.size < 0) {
            const char *response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
            close(client_fd);
            continue;
        }

        char *headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n\r\n"
            "%s";

        char response[strlen(headers) + file_data.size + 50];

        int respone_len = snprintf(response, sizeof(response), headers,
                                   request_accept, file_data.size, file_data.buffer);

        printf("%s\n", file_data.buffer);

        send(client_fd, response, respone_len, 0);

        close(client_fd);
        free(file_data.buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 3 || argc < 3) {
        printf("help: [domain] [port]\n");
        return 0;
    }

    const char *domain = argv[1];
    const int port = atoi(argv[2]);

    int socket = create_socket(domain, port);
    if (socket < 0) {
        perror("Server creation failed");
        exit(1);
    }

    handle_requests(socket);
    close(socket);
    return 0;
}
