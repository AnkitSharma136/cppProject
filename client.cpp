#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>

const int BUFFER_SIZE = 1024;
const int PORT = 8888;

// Function to handle receiving messages from the server
void receiveMessages(int clientSocket, char* buffer,bool &exitFlag);

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    bool exitFlag = false;
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

    // std::cout << "Connected to server. Enter your name: ";
    // std::getline(std::cin, name);

    //Authentication
    // Get username and password from user
    std::string username, password;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    std::cout << "Enter your password: ";
    std::getline(std::cin, password);


    // Send username and password to server for authentication as a single string
    std::string credentials = username + ":" + password;
    send(clientSocket, credentials.c_str(), credentials.length(), 0);


    //Seperate Thread for receiving Message
    std::thread receiveThread(receiveMessages, clientSocket, buffer, std::ref(exitFlag));

    // std::cout << "Start typing your messages...\n";

    // Wait for authentication response from server
    char authResponse[BUFFER_SIZE];
    int bytesReceived=recv(clientSocket, authResponse, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error: Failed to receive authentication response from server\n";
        close(clientSocket);
        return EXIT_FAILURE;
    }
    authResponse[bytesReceived] = '\0';
    std::string authResult(authResponse);
    // std::cout<<"authResult"<<authResult<<std::endl;
    if (authResult == "authenticated") {
        std::cout << "Start typing your messages...\n";
    } else {
        std::cerr << "Authentication failed. Exiting...\n";
        return EXIT_FAILURE;
    }

    while (true) {
        std::cout << " > ";
        std::string message;
        std::getline(std::cin, message);

        // Terminate Client
        if(message == "#exit"){
            send(clientSocket, message.c_str(), message.length(), 0);
            exitFlag = true;
            break;
        };

        // Construct message with name
        std::string fullMessage = name + ": " + message;

        // Send message to server
        send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);

        // Receive response from server
        // int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        // if (bytesReceived <= 0) {
        //     std::cerr << "Server disconnected\n";
        //     break;
        // }

        // buffer[bytesReceived] = '\0';
        // std::cout << buffer << std::endl; 
    }
    receiveThread.join();

    close(clientSocket);
    return EXIT_SUCCESS;
}

void receiveMessages(int clientSocket, char* buffer,bool &exitFlag) {
    while (true) {

        if(exitFlag){
            std::cout<<"Exit Signal recieved"<<std::endl;
            break;
        }
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Server disconnected\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }
}
