#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char *read_html(const char *filename) {
    FILE *file;
    file = fopen(filename, "rb");

    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        perror("Malloc failed");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, sizeof(char), file_size, file);

    if (read_size != file_size) {
        perror("Failed to open entire file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[read_size] = '\0';

    fclose(file);
    return buffer;
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

    printf("Server listening on %d\n", port);

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

        char *headers =
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld; "
            "charset=UTF-8\r\n\r\n%s";
        char *html = read_html("index.html");

        if (!html) {
            const char *response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            send(client_fd, response, strlen(response), 0);
            close(client_fd);
            continue;
        }

        char response[strlen(headers) + strlen(html)];
        snprintf(response, sizeof(response), headers, strlen(html), html);

        send(client_fd, response, strlen(response), 0);

        close(client_fd);
        free(html);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        printf("To many arguments\n");
        return 0;
    } else if (argc < 3) {
        printf("What the fuck are you doin\n");
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
