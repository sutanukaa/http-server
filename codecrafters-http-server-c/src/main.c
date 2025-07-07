#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

int check_gzip_support(char *accept_encoding_line);
int gzip_compress(const char* input, int input_len, char* output, int* output_len);
int should_keep_connection_open(char *request);
void handle_client_connection(int client_fd, char *directory);

#ifdef _WIN32
DWORD WINAPI client_thread(LPVOID arg);
#else
void* client_thread(void* arg);
#endif

// Structure to pass data to thread
typedef struct {
    int client_fd;
    char *directory;
} client_data_t;

int main(int argc, char *argv[]) {
    char *directory = NULL;
    for (int i =1;i<argc; i++){
        if(strcmp(argv[i], "--directory")==0){
            if (i + 1 < argc) {
                directory = argv[i + 1];
                i++; 
            } 
        }
    }
    if (directory==NULL) {
        directory = "/tmp"; 
    }
    
    printf("Using directory: %s\n", directory);


    
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    
    printf("Logs from your program will appear here!\n");
    printf("Hello, World!\n");
    
#ifdef _WIN32
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif


    
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
    
    printf("Waiting for clients to connect...\n");
    client_addr_len = sizeof(client_addr);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd < 0) {
#ifdef _WIN32
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
#else
            printf("Accept failed: %s\n", strerror(errno));
            continue;
#endif
        }
        
        printf("Client connected\n");
        
        // Create thread data structure
        client_data_t *client_data = malloc(sizeof(client_data_t));
        client_data->client_fd = client_fd;
        client_data->directory = directory;
        
        // Create thread to handle client
#ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, client_thread, client_data, 0, NULL);
        if (thread == NULL) {
            printf("Failed to create thread\n");
            close(client_fd);
            free(client_data);
        } else {
            CloseHandle(thread); // We don't need to wait for the thread
        }
#else
        pthread_t thread;
        if (pthread_create(&thread, NULL, client_thread, client_data) != 0) {
            printf("Failed to create thread\n");
            close(client_fd);
            free(client_data);
        } else {
            pthread_detach(thread); // We don't need to wait for the thread
        }
#endif
    }
    
    close(server_fd);

#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}

// Thread function to handle client connections
#ifdef _WIN32
DWORD WINAPI client_thread(LPVOID arg) {
#else
void* client_thread(void* arg) {
#endif
    client_data_t *data = (client_data_t*)arg;
    handle_client_connection(data->client_fd, data->directory);
    free(data);
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Handle client connection with persistent connection support
void handle_client_connection(int client_fd, char *directory) {
    printf("Thread handling client connection\n");
    
    // Inner loop to handle multiple requests on same connection
    int keep_connection_open = 1;
    while (keep_connection_open) {
        // Read the HTTP request
        char read_buffer[1024];
        memset(read_buffer, 0, sizeof(read_buffer));
        int bytes_read = recv(client_fd, read_buffer, sizeof(read_buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            // Connection closed by client or error
            if (bytes_read == 0) {
                printf("Connection closed by client\n");
            } else {
                printf("Error reading from client: %d\n", bytes_read);
            }
            break;
        }
        
        printf("Received request:\n%s\n", read_buffer);
        
        int supports_gzip = 0;
        char *accept_encoding = strstr(read_buffer, "Accept-Encoding:");
        if (accept_encoding) {
            supports_gzip = check_gzip_support(accept_encoding);
            if (supports_gzip) {
                printf("Client supports gzip compression\n");
            }
        }
        
        // Check if client wants to keep connection open
        int will_keep_open = should_keep_connection_open(read_buffer);
        const char* connection_header = will_keep_open ? "keep-alive" : "close";
        
        // Check if it's a GET request for root path
        if (strstr(read_buffer, "GET / ") != NULL) {
            // Send 200 OK response
            const char* message = "Hello, World!";
            char response[1024];
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n%s", 
                    connection_header, (int)strlen(message), message);
            send(client_fd, response, strlen(response), 0);
            printf("Sent root response\n");
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
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n%s", 
                        connection_header, (int)strlen(user_agent), user_agent);
                send(client_fd, response, strlen(response), 0);
                printf("Sent User-Agent response: %s\n", user_agent);
            } 
        }
        else if (strstr(read_buffer, "GET /echo/") != NULL) {
            // Extract the string after /echo/
            char *echo_start = strstr(read_buffer, "GET /echo/");
            echo_start += strlen("GET /echo/");
            char *echo_end = strstr(echo_start, " ");
            if (echo_end) {
                *echo_end = '\0';  // Null-terminate the string
            }
            
            char response[1024];
            
            if (supports_gzip) {
                // Compress the data
                char compressed[1024];
                int compressed_length = sizeof(compressed);
                
                if (gzip_compress(echo_start, strlen(echo_start), compressed, &compressed_length) == Z_OK) {
                    // Compression successful
                    int header_length = sprintf(response, 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Encoding: gzip\r\n"
                        "Connection: %s\r\n"
                        "Content-Length: %d\r\n\r\n", 
                        connection_header, compressed_length);
                    
                    send(client_fd, response, header_length, 0);
                    send(client_fd, compressed, compressed_length, 0);
                    printf("Sent gzip compressed echo response: %s (%d -> %d bytes)\n", 
                           echo_start, (int)strlen(echo_start), compressed_length);
                } else {
                    // Compression failed, send uncompressed
                    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n%s", 
                            connection_header, (int)strlen(echo_start), echo_start);
                    send(client_fd, response, strlen(response), 0);
                    printf("Compression failed, sent uncompressed echo response: %s\n", echo_start);
                }
            } else {
                // Send uncompressed response
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: %s\r\nContent-Length: %d\r\n\r\n%s", 
                        connection_header, (int)strlen(echo_start), echo_start);
                send(client_fd, response, strlen(response), 0);
                printf("Sent uncompressed echo response: %s\n", echo_start);
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
                        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nConnection: %s\r\nContent-Length: %ld\r\n\r\n", 
                                connection_header, file_size);
                        fclose(file);
                        // Send file content
                        send(client_fd, response, strlen(response), 0);  // Send headers
                        send(client_fd, file_content, file_size, 0);   // Send raw file content
                        free(file_content);
                        printf("Sent file: %s\n", name);
                    } else {
                        // Send 404 Not Found response if file not found
                        char not_found_response[1024];
                        sprintf(not_found_response, "HTTP/1.1 404 Not Found\r\nConnection: %s\r\n\r\n", connection_header);
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
                        char response[1024];
                        sprintf(response, "HTTP/1.1 201 Created\r\nConnection: %s\r\n\r\n", connection_header);
                        send(client_fd, response, strlen(response), 0);
                        printf("File created: %s\n", filepath);
                    } else {
                        char error_response[1024];
                        sprintf(error_response, "HTTP/1.1 500 Internal Server Error\r\nConnection: %s\r\n\r\n", connection_header);
                        send(client_fd, error_response, strlen(error_response), 0);
                    }
                    free(body);
                }
            }
            else {
                // Send 404 Not Found response
                char not_found_response[1024];
                sprintf(not_found_response, "HTTP/1.1 404 Not Found\r\nConnection: %s\r\n\r\n", connection_header);
                send(client_fd, not_found_response, strlen(not_found_response), 0);
                printf("Sent 404 Not Found response\n");
            }
        
        // Set keep_connection_open based on our decision
        keep_connection_open = will_keep_open;
        
        // Add appropriate Connection header based on decision
        if (!keep_connection_open) {
            printf("Will close connection after this response\n");
        }
    }
    
    printf("Closing connection\n");
    close(client_fd);
}

// Check if gzip is in the comma-separated list
int check_gzip_support(char *accept_encoding_line) {
    
    char *values_start = accept_encoding_line + strlen("Accept-Encoding:");
    
    
    while (*values_start == ' ' || *values_start == '\t') {
        values_start++;
    }
    
    
    char *current = values_start;
    while (current && *current != '\r' && *current != '\n') {
    
        while (*current == ' ' || *current == '\t') {
            current++;
        }
        
        
        if (strncmp(current, "gzip", 4) == 0) {
        
            char next_char = *(current + 4);
            if (next_char == ',' || next_char == ' ' || next_char == '\t' || 
                next_char == '\r' || next_char == '\n' || next_char == '\0') {
                return 1; 
            }
        }
        
        while (*current != ',' && *current != '\r' && *current != '\n' && *current != '\0') {
            current++;
        }
        
        
        if (*current == ',') {
            current++;
        }
    }
    
    return 0; 
}

int gzip_compress(const char* input, int input_len, char* output, int* output_len) {
    z_stream strm;
    int ret;
    
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    // Initialize deflate with gzip format
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
                      15 + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return ret; 
    }
    
    strm.avail_in = input_len;
    strm.next_in = (unsigned char*)input;
    strm.avail_out = *output_len;
    strm.next_out = (unsigned char*)output;
    
    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return ret; 
    }
    
    *output_len = *output_len - strm.avail_out;
    deflateEnd(&strm);
    return Z_OK;
}

int should_keep_connection_open(char *request) {
    // Check HTTP version first
    int is_http_11 = (strstr(request, "HTTP/1.1") != NULL);
    
    // Check for Connection header
    char *connection_header = strstr(request, "Connection:");
    if (connection_header) {
        // Move past "Connection:" and any whitespace
        connection_header += strlen("Connection:");
        while (*connection_header == ' ' || *connection_header == '\t') {
            connection_header++;
        }
        
        // Check if it contains "close"
        if (strncmp(connection_header, "close", 5) == 0) {
            char next_char = *(connection_header + 5);
            if (next_char == '\r' || next_char == '\n' || next_char == ' ' || 
                next_char == '\t' || next_char == '\0') {
                return 0; // Close connection
            }
        }
        
        // Check if it contains "keep-alive"
        if (strncmp(connection_header, "keep-alive", 10) == 0) {
            char next_char = *(connection_header + 10);
            if (next_char == '\r' || next_char == '\n' || next_char == ' ' || 
                next_char == '\t' || next_char == '\0') {
                return 1; // Keep connection open
            }
        }
    }
    
    // Default behavior: keep connection open for HTTP/1.1, close for HTTP/1.0
    return is_http_11 ? 1 : 0;
}
