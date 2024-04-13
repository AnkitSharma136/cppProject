#include<iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<cstdlib>
#include<cstring>
#include<arpa/inet.h>
#include<vector>
#include<thread>
#include<map>
#include<algorithm>

const int PORT = 9000;
const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 1024;

struct room{
    std::vector<int> client;
    std::string roomPassword;
};

std::map<std::string,room> rooms;
std::map<std::string, std::string> userCredentials;

bool authenticateUser(const std::string& userName, const std::string& userassword) {
    if(userCredentials.find(userName)==userCredentials.end()){
        userCredentials[userName]=userassword;
        return true;
    }
    auto it = userCredentials.find(userName);
    if (it->second == userassword) {
        return true;  // Authentication successful
    }
    return false;     // Authentication failed
}

void handleClient(int clientFd,std::string roomName, std::string password, std::string clientName);

int main(){
    int serverFd;
    int clientFd;

    struct sockaddr_in serverAddress , clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    //Socket Creation
    serverFd = socket(AF_INET,SOCK_STREAM,0);

    if(serverFd == -1) {
        std::cerr<<"Failed to create Socket";
        return EXIT_FAILURE;
    }

    //Socket Preparation
    serverAddress.sin_family =AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port=htons(PORT);

    //Socket Binding
    if(bind(serverFd , (struct sockaddr*)&serverAddress,sizeof(serverAddress))== -1){
        std::cerr<<"Failed to bind Socket";
        return EXIT_FAILURE;
    }

    //Socket Listening
    if(listen(serverFd,MAX_CLIENTS)==-1){
        std::cerr<<"Failed to listen";
        return EXIT_FAILURE;
    }
    else
        std::cout<< "Listening on Port : " << PORT <<" Waiting for connections..." <<std::endl;

    while(true){
        clientFd = accept(serverFd,(struct sockaddr*) &clientAddress, &clientAddressLength);

        if(clientFd == -1){
            std::cerr<<"Client Accept Failed";
            return EXIT_FAILURE;
        }

        std::cout<<"New Connection Established at IP and PORT :"<< inet_ntoa(clientAddress.sin_addr) <<"  " <<ntohs(clientAddress.sin_port)<<std::endl;

        // Authenticate the client
        char credentials[BUFFER_SIZE];
        recv(clientFd, credentials, BUFFER_SIZE, 0);

        std::string credentialsStr(credentials);
        size_t delimiterPos = credentialsStr.find(':');
        std::string userName = credentialsStr.substr(0, delimiterPos);
        std::string UserPassword = credentialsStr.substr(delimiterPos + 1);

        if (!authenticateUser(userName, UserPassword)) {
            std::cerr << "Authentication failed for " << userName << std::endl;
            close(clientFd);
            continue;
        }


        std::cout << "User " << userName << " authenticated successfully.\n";
        // Authentication successful, send authentication result to the client
        send(clientFd, "authenticated", strlen("authenticated"), 0);

        char dataBuffer[BUFFER_SIZE];
        int bytesReceived = recv(clientFd,dataBuffer,BUFFER_SIZE,0);
        if (bytesReceived <= 0) {
            std::cerr << "Error: Failed to receive data from client\n";
            close(clientFd);
            continue;
        }
        dataBuffer[bytesReceived] = '\0';
        
        std::string data(dataBuffer);
        size_t position1 = data.find(',');
        size_t position2 =data.find(',',position1 + 1);

        if (position1 == std::string::npos || position2 == std::string::npos) {
            std::cerr << "Error: Invalid data format from client\n";
            close(clientFd);
            continue;
        }

        std::string roomName = data.substr(0, position1);
        std::string password = data.substr(position1 + 1, position2 - position1 - 1);
        std::string clientName = data.substr(position2 + 1);

        if(rooms.find(roomName)==rooms.end()){
            room newRoom;
            newRoom.roomPassword = password;
            rooms[roomName] = newRoom;
        }

        std::thread clientThread(handleClient, clientFd, roomName, password, clientName);
        clientThread.detach();
    }

    close(serverFd);
    return EXIT_SUCCESS;

}

void handleClient(int clientFd, std::string roomName, std::string password, std::string clientName) {
    room &currentRoom = rooms[roomName];

    if (!password.empty() && currentRoom.roomPassword != password) {
        std::cerr << "Invalid password for room = " << roomName << std::endl;
        close(clientFd);
        return;
    }

    currentRoom.client.push_back(clientFd);

    while (true) {
        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(clientFd, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {
            std::cout << "Client " << clientName << " disconnected from room " << roomName << std::endl;
            close(clientFd);
            currentRoom.client.erase(std::remove(currentRoom.client.begin(), currentRoom.client.end(), clientFd), currentRoom.client.end());
            return;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Client: " << clientName << " in room = " << roomName << " says: " << buffer << std::endl;
        
        std::string message = clientName + ": " + std::string(buffer, bytesReceived); 
        
        for (int otherClient : currentRoom.client) {
            if (otherClient != clientFd) {
                send(otherClient, message.c_str(), message.size(), 0);
            }
        }
    }
}

