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

void receiveMessages(int clientFd);

int main() {
    int clientFd;
    struct sockaddr_in serverAddress;

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd == -1) {
        cerr << "Failed to create Client Socket";
        return EXIT_FAILURE;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PORT);

    if (connect(clientFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to connect to server";
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];

    //Authentication
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
    authResponse[bytesReceived] = '\0';
    string authResult(authResponse);
    if (authResult == "authenticated") {
        // cout << "Start typing your messages...\n";
    } else {
        cerr << "Authentication failed. Exiting...\n";
        return EXIT_FAILURE;
    }

    cout << "Enter the (roomName,Password,ClientName) comma seperated" << endl;
    cin.getline(buffer, BUFFER_SIZE);

    send(clientFd, buffer, strlen(buffer), 0);

    thread receiveThread(receiveMessages, clientFd);
    cout << "===========Start Typing Your Message===========" << endl;

    while (true) {
        cout << "=>> ";
        string message;
        getline(cin, message);

        int bytesSent = send(clientFd, message.c_str(), message.length(), 0);
        if (bytesSent == -1) {
            cerr << "Server Disconnected , Message not Sent";
            break;
        }
    }

    receiveThread.join();
    close(clientFd);
    return EXIT_SUCCESS;
}

void receiveMessages(int clientFd) {
    while (true) {
        char buffer[BUFFER_SIZE];

        int bytesReceived = recv(clientFd, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            cerr << "Server Disconnected";
            break;
        }
        buffer[BUFFER_SIZE] = '\0';
        cout << buffer << endl;
        cout << "=>>> ";
    }
}
