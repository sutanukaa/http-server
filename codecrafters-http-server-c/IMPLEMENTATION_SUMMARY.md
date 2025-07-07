# Multi-threaded HTTP Server Implementation

## ✅ COMPLETED FEATURES

### 1. **Basic HTTP Server Functionality**
- ✅ GET `/` - Returns "Hello, World!" 
- ✅ GET `/user-agent` - Returns User-Agent header value
- ✅ GET `/echo/{str}` - Echoes the string back
- ✅ GET `/files/{filename}` - Serves files from directory
- ✅ POST `/files/{filename}` - Creates files in directory
- ✅ Command-line `--directory` flag support (default: /tmp)

### 2. **HTTP/1.1 Features**
- ✅ Proper HTTP/1.1 response headers
- ✅ Content-Type headers for all responses
- ✅ Content-Length headers for all responses
- ✅ Connection headers (keep-alive/close)

### 3. **Accept-Encoding & Gzip Compression**
- ✅ Parse Accept-Encoding header with multiple comma-separated values
- ✅ Robust gzip detection (complete word matching)
- ✅ Gzip compression for /echo/{str} responses when requested
- ✅ Proper Content-Encoding: gzip header
- ✅ Correct Content-Length for compressed data

### 4. **Persistent HTTP/1.1 Connections (Keep-Alive)**
- ✅ Multiple requests per TCP connection
- ✅ Connection header parsing (close vs keep-alive)
- ✅ HTTP/1.1 vs HTTP/1.0 default behavior
- ✅ Proper connection lifecycle management
- ✅ **Explicit Connection: close header support**
- ✅ **Server responds with Connection: close when requested**
- ✅ **TCP connection closed after Connection: close response**

### 5. **⭐ CONCURRENT CONNECTIONS (NEW)**
- ✅ **Multi-threaded server architecture**
- ✅ **Each connection handled in separate thread**
- ✅ **Independent request processing per connection**
- ✅ **Concurrent TCP connection support**
- ✅ **Thread-safe connection handling**

## 📋 IMPLEMENTATION DETAILS

### **Threading Architecture:**
- **Windows**: Uses `CreateThread()` with `WINAPI` thread functions
- **Linux/Unix**: Uses `pthread_create()` with POSIX threads
- **Thread Data**: Each thread receives client socket and directory path
- **Memory Management**: Thread data is allocated per connection and freed when thread ends
- **Thread Lifecycle**: Threads are detached (don't need to be joined)

### **Connection Management:**
- **Connection Header Detection**: Case-insensitive parsing of `Connection: close` and `Connection: keep-alive`
- **Response Headers**: Server includes appropriate `Connection` header in response
- **Connection Lifecycle**: 
  - Keep-alive: Connection remains open for multiple requests
  - Close: Connection is closed after sending the response with `Connection: close`
- **HTTP Version Defaults**: HTTP/1.1 defaults to keep-alive, HTTP/1.0 defaults to close

### **Connection Flow:**
1. **Main thread** accepts incoming connections
2. **New thread** created for each accepted connection
3. **Thread handles** multiple requests on that connection (keep-alive)
4. **Connection closed** when client sends `Connection: close` or on error
5. **Thread terminates** when connection closes
6. **Process repeats** for new connections

### **Key Functions:**
- `client_thread()` - Thread entry point (Windows/Linux compatible)
- `handle_client_connection()` - Handles persistent connection with multiple requests
- `should_keep_connection_open()` - Determines connection keep-alive behavior (supports Connection: close)

### **Thread Safety:**
- Each connection has its own thread and socket
- No shared state between connections
- Directory path is passed safely to each thread
- File operations are isolated per thread

## 🔧 COMPILATION

### **Windows (MinGW):**
```bash
gcc -std=c11 src\main.c -o server.exe -lws2_32 -lwsock32 -lz
```

### **Linux:**
```bash
gcc -std=c11 src/main.c -o server -lpthread -lz
```

## 🚀 USAGE

```bash
# Start server with default directory (/tmp)
./server

# Start server with custom directory
./server --directory /path/to/files
```

## 🎯 TESTING CONCURRENT CONNECTIONS

You can now test with multiple simultaneous connections:

```bash
# Terminal 1
curl -H "Connection: keep-alive" http://localhost:4221/

# Terminal 2 (simultaneously)
curl -H "Connection: keep-alive" http://localhost:4221/echo/hello

# Terminal 3 (simultaneously)  
curl -H "Accept-Encoding: gzip" http://localhost:4221/echo/world
```

All connections will be handled concurrently, each in its own thread, with proper persistent connection support.

## 🧪 CONNECTION: CLOSE TEST

### Test Command:
```bash
curl --http1.1 -v http://localhost:4221/echo/orange --next http://localhost:4221/ -H "Connection: close"
```

### Expected Behavior:
1. **First Request** (`/echo/orange`): 
   - No Connection header → Server responds with `Connection: keep-alive`
   - Connection stays open for next request

2. **Second Request** (`/` with `Connection: close`):
   - Server detects `Connection: close` header
   - Server responds with `Connection: close` 
   - Server closes TCP connection after response

### Server Logs:
```
Client connected
Thread handling client connection
No explicit Connection header, using HTTP version default (HTTP/1.1: 1)
Sent uncompressed echo response: orange
Found Connection: close header
Client requested connection close
Sent root response
Will close connection after this response
Closing connection
```

## ✅ REQUIREMENTS SATISFIED

1. ✅ **Handle multiple concurrent TCP connections**
2. ✅ **Keep each connection open for multiple requests**
3. ✅ **Process requests independently on each connection**
4. ✅ **Return appropriate responses for all requests**
5. ✅ **Each connection handled independently**
6. ✅ **Server maintains state of each connection separately**
7. ✅ **Requests on different connections processed concurrently**
