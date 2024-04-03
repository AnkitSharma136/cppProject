#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

constexpr int BUFFER_SIZE = 1024;
constexpr int PORT = 8080;

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    std::string name;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return EXIT_FAILURE;
    }

    // Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        return EXIT_FAILURE;
    }

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error: Connection failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Connected to server. Enter your name: ";
    std::getline(std::cin, name);

    std::cout << "Start typing your messages...\n";

    while (true) {
        std::cout << name << " > ";
        std::string message;
        std::getline(std::cin, message);

        // Construct message with name
        std::string fullMessage = name + ": " + message;

        // Send message to server
        send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);

        // Receive response from server
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Server disconnected\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }

    close(clientSocket);
    return EXIT_SUCCESS;
}
