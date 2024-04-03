#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

const int BUFFER_SIZE = 1024;
const int MAX_CLIENTS = 10;
const int PORT = 5555;

std::vector<int> clientSockets(MAX_CLIENTS, 0);

void handleClient(int clientSocket, int clientIndex) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cout << "Client " << clientIndex << " disconnected.\n";
            close(clientSocket);
            clientSockets[clientIndex] = 0;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Client " << clientIndex << " says: " << buffer << std::endl;

        // Broadcast the message to all clients except the sender
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] != 0 && i != clientIndex) {
                send(clientSockets[i], buffer, bytesReceived, 0);
            }
        }
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return EXIT_FAILURE;
    }

    // Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error: Bind failed\n";
        return EXIT_FAILURE;
    }

    // Listen
    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        std::cerr << "Error: Listen failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server started. Waiting for connections...\n";

    int clientIndex = 0;

    while (true) {
        // Accept connection from an incoming client
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            std::cerr << "Error: Accept failed\n";
            continue;
        }

        std::cout << "New connection established. Client IP: " << inet_ntoa(clientAddr.sin_addr)
                  << ", Port: " << ntohs(clientAddr.sin_port) << std::endl;

        // Add client socket to the vector
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clientSockets[i] == 0) {
                clientSockets[i] = clientSocket;
                clientIndex = i;
                break;
            }
        }

        // Create thread to handle client
        std::thread clientThread(handleClient, clientSocket, clientIndex);
        clientThread.detach(); // Detach the thread to allow it to run independently
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
