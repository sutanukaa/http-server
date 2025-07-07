# Connection Close Test

## Test Case: Connection: close header handling

This test verifies that the server properly handles the `Connection: close` header.

### Expected Behavior:

1. **First request** (no Connection header): Server keeps connection open, responds with `Connection: keep-alive`
2. **Second request** (with `Connection: close`): Server closes connection after response, responds with `Connection: close`

### Test Commands:

```bash
# Test with curl (simulates the tester)
curl --http1.1 -v http://localhost:4221/echo/orange --next http://localhost:4221/ -H "Connection: close"
```

### Expected Server Logs:

```
Client connected
Thread handling client connection
Received request:
GET /echo/orange HTTP/1.1
Host: localhost:4221
...

No explicit Connection header, using HTTP version default (HTTP/1.1: 1)
Sent uncompressed echo response: orange

Received request:
GET / HTTP/1.1
Host: localhost:4221
Connection: close
...

Found Connection: close header
Client requested connection close
Sent root response
Will close connection after this response
Closing connection
```

### Key Points:

1. Server detects `Connection: close` header (case-insensitive)
2. Server responds with `Connection: close` in the response
3. Server closes the TCP connection after sending the response
4. Each connection runs in its own thread independently

### Response Headers:

**First Response:**
```
HTTP/1.1 200 OK
Content-Type: text/plain
Connection: keep-alive
Content-Length: 6

orange
```

**Second Response:**
```
HTTP/1.1 200 OK
Content-Type: text/plain
Connection: close
Content-Length: 13

Hello, World!
```

After the second response, the TCP connection is closed by the server.
