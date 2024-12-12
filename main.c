#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int create_socket() {
    int server_fd;
    struct sockaddr_in server_addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    if (listen(server_fd, 128) < 0) {
        perror("Failed Listen");
        return -1;
    }

    printf("Server listening on %d\n", 8080);

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

        char *response =
            "HTTP/1.1 200 OK\r\nContent-Type:"
            "text/plain\r\nContent-Length: 5\r\n\r\nhello";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
    }
}

void start() {
    int socket = create_socket();
    handle_requests(socket);
    close(socket);
}

int main() {
    start();
    return 0;
}
