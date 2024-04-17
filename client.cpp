#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <fstream>

using namespace std;

const int PORT = 9000;
const int BUFFER_SIZE = 1024;

// Function prototypes
void receiveMessages(int clientFd);
int authenticateUser(int clientFd);
int createOrJoinRoom(int clientFd);

int main() {
    int clientFd;  // Client File Descriptor
    struct sockaddr_in serverAddress; // Server Address to which client will connect  

    // Socket Creation
    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd == -1) {
        cerr << "Failed to create Client Socket";
        return EXIT_FAILURE;
    }

    // Set serverAddress Fields
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PORT);

    // Connecting to server
    if (connect(clientFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to connect to server";
        return EXIT_FAILURE;
    }

    // Authenticate User 
    if(authenticateUser(clientFd) == EXIT_FAILURE){
        cerr<< "Failed to authenticate user";
        return EXIT_FAILURE;
    }

    cout<<"======================================================="<<endl;

    // Create Or Join room
    if(createOrJoinRoom(clientFd) == EXIT_FAILURE){
        cerr << "Access Denied by Server , Wrong Password for existing room\n";
        cout<<endl;
        return EXIT_FAILURE;
    }

    // Start receiving messages in a separate thread
    thread receiveThread(receiveMessages, clientFd);
    cout << "===========Start Typing Your Message===========" << endl;

    // Continuously read input from user and send to server
    while (true) {
        string message;
        getline(cin, message);

        int bytesSent = send(clientFd, message.c_str(), message.length(), 0);
        if (bytesSent == -1) {
            cerr << "Server Disconnected , Message not Sent";
            break;
        }
    }

    // Join the receive thread and close the client socket when done
    receiveThread.join();
    close(clientFd);
    return EXIT_SUCCESS;
}

// Function to continuously receive messages from server
void receiveMessages(int clientFd) {
    while (true) {
        char buffer[BUFFER_SIZE];

        int bytesReceived = recv(clientFd, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            cerr << "Server Disconnected";
            break;
        }

        // Null-terminate the buffer to convert it to a string
        buffer[bytesReceived] = '\0';
        cout << buffer << endl;
    }
}

// Function to authenticate the user with the server
int authenticateUser(int clientFd){
    char buffer[BUFFER_SIZE];

    // Get username and password from user
    string username, password;
    cout << "Enter your username: ";
    getline(cin, username);
    cout << "Enter your password: ";
    getline(cin, password);

    // Send username and password to server for authentication as a single string
    string credentials = username + ":" + password;
    send(clientFd, credentials.c_str(), credentials.length(), 0);

    // Wait for authentication response from server
    char authResponse[BUFFER_SIZE];
    int bytesReceived = recv(clientFd, authResponse, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        cerr << "Error: Failed to receive authentication response from server\n";
        close(clientFd);
        return EXIT_FAILURE;
    }

    // Null-terminate the received data to convert it to a string
    authResponse[bytesReceived] = '\0';
    string authResult(authResponse);

    if (authResult ==  "authenticated")  
        return EXIT_SUCCESS;

    return EXIT_FAILURE;
}

// Function to create or join a room on the server
int createOrJoinRoom(int clientFd){
    char roomName[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    // Prompt the user to enter the roomName
    cout << "Enter the roomName: ";
    cin.getline(roomName, BUFFER_SIZE);

    // Prompt the user to enter the Password
    cout << "Enter the Password: ";
    cin.getline(password, BUFFER_SIZE);

    // Create a string in the format "roomName,Password"
    string buffer = string(roomName) + ":" + string(password);

    // Send the roomName and Password to the server
    send(clientFd, buffer.c_str(), buffer.length(), 0);

    // Receive the response from the server
    char recvBuffer[BUFFER_SIZE];
    int bytesReceived = recv(clientFd, recvBuffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        cerr << "Error: Failed to receive response from server\n";
        return EXIT_FAILURE;
    }

    // Null-terminate the received data to convert it to a string
    recvBuffer[bytesReceived] = '\0';

    // Process the response
    string response(recvBuffer);
    if (response == "Access Denied")
        return EXIT_FAILURE;
       
    return EXIT_SUCCESS;
}
