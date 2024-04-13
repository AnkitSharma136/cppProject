#include<sys/socket.h>
#include<iostream>
#include<arpa/inet.h>
#include<cstdlib>
#include<cstring>
#include<thread>
#include<unistd.h>

const int PORT = 9000;
const int BUFFER_SIZE = 1024;

void receiveMessages(int clientFd);

int main(){
    int clientFd;
    struct sockaddr_in serverAddress;

    clientFd = socket(AF_INET,SOCK_STREAM,0);
    if(clientFd == -1){
        std::cerr<<"Failed to create Client Socket";
        return EXIT_FAILURE;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port=htons(PORT);

    if(connect(clientFd,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
        std::cerr<<"Failed to connect to server";
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];

    //Authentication
    // Get username and password from user
    std::string username, password;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    std::cout << "Enter your password: ";
    std::getline(std::cin, password);

    // Send username and password to server for authentication as a single string
    std::string credentials = username + ":" + password;
    send(clientFd, credentials.c_str(), credentials.length(), 0);

    // Wait for authentication response from server
    char authResponse[BUFFER_SIZE];
    int bytesReceived=recv(clientFd, authResponse, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error: Failed to receive authentication response from server\n";
        close(clientFd);
        return EXIT_FAILURE;
    }
    authResponse[bytesReceived] = '\0';
    std::string authResult(authResponse);
    // std::cout<<"authResult"<<authResult<<std::endl;
    if (authResult == "authenticated") {
        //std::cout << "Start typing your messages...\n";
    } else {
        std::cerr << "Authentication failed. Exiting...\n";
        return EXIT_FAILURE;
    }

    std::cout<<"Enter the (roomName,Password,ClientName) comma seperated"<<std::endl;
    std::cin.getline(buffer,BUFFER_SIZE);

    send(clientFd,buffer,strlen(buffer),0);

    std::thread receiveThread(receiveMessages,clientFd);
    std::cout<<"===========Start Typing Your Message==========="<<std::endl;

    while(true){
        std::cout<<"=>> ";
        std::string message;
        std::getline(std::cin,message);

        int bytesSent = send(clientFd , message.c_str() , message.length(),0);
        if(bytesSent == -1){
            std::cerr<<"Server Disconnected , Message not Sent";
            break;
        }
    }

    receiveThread.join();
    close(clientFd);
    return EXIT_SUCCESS;
}
void receiveMessages(int clientFd){
    while(true){
        char buffer[BUFFER_SIZE];

        int bytesReceived = recv(clientFd,buffer,BUFFER_SIZE,0);
        if(bytesReceived <= 0){
            std::cerr<<"Server Disconnected";
            break;;
        }
        buffer[BUFFER_SIZE] = '\0';
        std::cout<<buffer<<std::endl;
        //std::cout<<"=>>> ";
    }
}