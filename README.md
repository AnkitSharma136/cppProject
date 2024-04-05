Simple Client Server Chatroom in C++ using socket Programming and Multi-threading.

Design:
1. Server will bind itself to port 5555 and listen for incoming connection.
2. Multiple Clients Can connect to the server.
3. File descriptor for every client is stored in vector(ClientSockets).
4. Multiple clients are handled by server through Multi-threading.
5. If client message == "#exit" Terminate client.
