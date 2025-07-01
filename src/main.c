#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <unistd.h>
#endif

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");
    printf("Hello, World!\n");
    // send(id, "HTTP/1.1 200 OK\r\n\r\n", 28, 0);

#ifdef _WIN32
    // Initialize Winsock on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    // Uncomment this block to pass the first stage
    
    int server_fd;
    socklen_t client_addr_len;
    struct sockaddr_in client_addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
#ifdef _WIN32
        printf("Socket creation failed: %d\n", WSAGetLastError());
#else
        printf("Socket creation failed: %s...\n", strerror(errno));
#endif
        return 1;
    }
    
    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
#ifdef _WIN32
        printf("SO_REUSEADDR failed: %d\n", WSAGetLastError());
#else
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
#endif
        return 1;
    }
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4221);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
#ifdef _WIN32
        printf("Bind failed: %d\n", WSAGetLastError());
#else
        printf("Bind failed: %s \n", strerror(errno));
#endif
        return 1;
    }
    
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
#ifdef _WIN32
        printf("Listen failed: %d\n", WSAGetLastError());
#else
        printf("Listen failed: %s \n", strerror(errno));
#endif
        return 1;
    }
    
    printf("Waiting for a client to connect...\n");
    client_addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd < 0) {
#ifdef _WIN32
        printf("Accept failed: %d\n", WSAGetLastError());
#else
        printf("Accept failed: %s\n", strerror(errno));
#endif
    } else {
        printf("Client connected\n");
        send(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
        close(client_fd);
    }
    
    close(server_fd);

#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
