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

int main(int argc, char *argv[]) {
    char *directory = NULL;
    for (int i =1;i<argc; i++){
        if(strcmp(argv[i], "--directory")==0){
            if (i + 1 < argc) {
                directory = argv[i + 1];
                i++; // Skip the next argument as it's the directory path
            } 
        }
    }
    if (directory==NULL) {
        directory = "/tmp"; // Default directory if not provided
    }
    // Print the directory to verify
    printf("Using directory: %s\n", directory);


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

    while (1){

    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd < 0) {
#ifdef _WIN32
        printf("Accept failed: %d\n", WSAGetLastError());
        continue;
#else
        printf("Accept failed: %s\n", strerror(errno));
        continue;
#endif
    } else {
        printf("Client connected\n");
        
        // Read the HTTP request
        char read_buffer[1024];
        memset(read_buffer, 0, sizeof(read_buffer));
        int bytes_read = recv(client_fd, read_buffer, sizeof(read_buffer) - 1, 0);
        
        if (bytes_read > 0) {
            printf("Received request:\n%s\n", read_buffer);
            
            // Check if it's a GET request for root path
            if (strstr(read_buffer, "GET / ") != NULL) {
                // Send 200 OK response
                const char* message = "Hello, World!";
char response[1024];
sprintf(response, "HTTP/1.1 200 OK\r\n\r\n%s", message);
send(client_fd, response, strlen(response), 0);
            } 
            else if (strstr(read_buffer, "GET /user-agent") != NULL) {
                // Extract User-Agent header
                char* user_agent = strstr(read_buffer, "User-Agent: ");
                if (user_agent) {
                    user_agent += strlen("User-Agent: ");
                    char* end_of_line = strstr(user_agent, "\r\n");
                    if (end_of_line) {
                        *end_of_line = '\0';  // Null-terminate the User-Agent string
                    }
                    
                    // Send 200 OK response with User-Agent
                    char response[1024];
                    sprintf(response, "HTTP/1.1 200 OK\r\n\r\n%s", user_agent);
                    send(client_fd, response, strlen(response), 0);
                    printf("Sent User-Agent response: %s\n", user_agent);
                } 
            }
                else if (strstr(read_buffer,"GET /files/")!=NULL) {
                    char *name = strstr(read_buffer, "GET /files/");
                    if (name) {
                        name+=strlen("GET /files/");
                        char *end = strstr(name, " ");
                        if (end) {
                            *end = '\0';  // Null-terminate the filename
                        }
                        // Open the requested file
                        // Build full path first
                            char filepath[512];
                            sprintf(filepath, "%s/%s", directory, name);  // Now opens "/tmp/foo"
                            FILE *file = fopen(filepath, "rb");
                        if (file) {
                            // Get file size
                            fseek(file, 0, SEEK_END);
                            long file_size = ftell(file);
                            fseek(file, 0, SEEK_SET);
                            char* file_content = malloc(file_size);
                            fread(file_content, 1, file_size, file);
                            char response[1024];
                            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n", file_size);
                            fclose(file);
                            // Send file content
                            send(client_fd, response, strlen(response), 0);  // Send headers
                            send(client_fd, file_content, file_size, 0);   // Send raw file content
                            free(file_content);
                            printf("Sent file: %s\n", name);
                        } else {
                            // Send 404 Not Found response if file not found
                            const char* not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
                            send(client_fd, not_found_response, strlen(not_found_response), 0);
                            printf("Sent 404 Not Found response for file: %s\n", name);
                        }
                    }
                } 
                else if (strstr(read_buffer, "POST /files/")!=NULL){
                    char *filename_start = strstr(read_buffer, "POST /files/");
                    if (filename_start){
                        filename_start+= strlen("POST /files/");
                        char *filename_end = strstr(filename_start, " ");
                        if (filename_end) {
                            *filename_end = '\0';  // Null-terminate the filename
                        }
                        // Extract the filename
                        char *length_str = strstr(read_buffer, "Content-Length: ");
                        int length = 0;
                        if (length_str){
                            length_str+= strlen("Content-Length: ");
                            length = atoi(length_str);
                        }
                        printf("Content-Length: %d\n", length);
                        
                        // Finding request body
                        char *body_start = strstr(read_buffer, "\r\n\r\n");
                        if (body_start) {
                            body_start+=4;
                        }
                        int body_read = bytes_read - (body_start - read_buffer);
                        int remaining_length = length - body_read;
                        char *body = malloc(length + 1);
                        memcpy(body, body_start, body_read);
                        if (remaining_length>0){
                            int additional_byte = recv(client_fd, body + body_read, remaining_length, 0);
                        }
                        // Building the full path
                        char filepath[512];
                        sprintf(filepath, "%s/%s", directory, filename_start);  
                        FILE *file = fopen(filepath, "wb");
                        if (file){
                            fwrite(body, 1, length, file);
                            fclose(file);
                            const char* response = "HTTP/1.1 201 Created\r\n\r\n";
                            send(client_fd, response, strlen(response), 0);
                            printf("File created: %s\n", filepath);
                        } else {
                            const char* error_response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                            send(client_fd, error_response, strlen(error_response), 0);
                        }
                        free(body);
                    }
                }





                
                else {
                    // Send 404 Not Found response if User-Agent not found
                    const char* not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
                    send(client_fd, not_found_response, strlen(not_found_response), 0);
                    printf("Sent 404 Not Found response\n");
                }
            }
            
            
            else {
                // Send 404 Not Found response
                const char* not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(client_fd, not_found_response, strlen(not_found_response), 0);
                printf("Sent 404 Not Found response\n");
            }
        }
        
        close(client_fd);
    }
}
    
    close(server_fd);

#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
