#include<iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<cstdlib>
#include<cstring>
#include<arpa/inet.h>
#include<vector>
#include<thread>
#include<map>
#include<fstream>
#include<algorithm>

using namespace std;

const int PORT = 9000;
const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 1024;

struct room{
    vector<int> client;
    string roomPassword;
};

map<string,room> rooms;
map<string, string> userCredentials;

void fillMap();
bool authenticateUser(const string& userName, const string& userPassword);
void handleClient(int clientFd,string roomName, string password, string clientName);

int main(){
    int serverFd;
    int clientFd;
    
    //Fill Map from text file for user Authentication
    fillMap();
    
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    //Socket Creation
    serverFd = socket(AF_INET,SOCK_STREAM,0);

    if(serverFd == -1) {
        cerr<<"Failed to create Socket";
        return EXIT_FAILURE;
    }

    //Socket Preparation
    serverAddress.sin_family =AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port=htons(PORT);

    //Socket Binding
    if(bind(serverFd , (struct sockaddr*)&serverAddress,sizeof(serverAddress))== -1){
        cerr<<"Failed to bind Socket";
        return EXIT_FAILURE;
    }

    //Socket Listening
    if(listen(serverFd,MAX_CLIENTS)==-1){
        cerr<<"Failed to listen";
        return EXIT_FAILURE;
    }
    else{
        cout<< "Listening on Port : " << PORT <<" Waiting for connections..." <<endl;
    }

    while(true){
        clientFd = accept(serverFd,(struct sockaddr*) &clientAddress, &clientAddressLength);

        if(clientFd == -1){
            cerr<<"Client Accept Failed";
            return EXIT_FAILURE;
        }

        cout<<"New Connection Established at IP and PORT :"<< inet_ntoa(clientAddress.sin_addr) <<"  " <<ntohs(clientAddress.sin_port)<<endl;

        // Authenticate the client
        char credentials[BUFFER_SIZE];
        int bytes = recv(clientFd, credentials, BUFFER_SIZE, 0);

        if (bytes <= 0) {
            cerr << "Error: Failed to receive data from client\n";
            close(clientFd);
            continue;
        }
        credentials[bytes] = '\0';

        cout<<credentials<<endl;

        string credentialsStr(credentials);
        size_t delimiterPos = credentialsStr.find(':');
        string userName = credentialsStr.substr(0, delimiterPos);
        string UserPassword = credentialsStr.substr(delimiterPos + 1);

        if (!authenticateUser(userName, UserPassword)) {
            cout<<userName<<" "<<UserPassword<<endl;
            cerr << "Authentication failed for " << userName << endl;
            send(clientFd,"Access Denied",strlen("Access Denied"),0);
            close(clientFd);
            continue;
        }


        cout << "User " << userName << " authenticated successfully.\n";
        // Authentication successful, send authentication result to the client
        send(clientFd, "authenticated", strlen("authenticated"), 0);

        char dataBuffer[BUFFER_SIZE];
        int bytesReceived = recv(clientFd,dataBuffer,BUFFER_SIZE,0);
        if (bytesReceived <= 0) {
            cerr << "Error: Failed to receive data from client\n";
            close(clientFd);
            continue;
        }
        dataBuffer[bytesReceived] = '\0';
        
        string data(dataBuffer);
        size_t position1 = data.find(':');
        string roomName = data.substr(0, position1);
        string password = data.substr(position1 + 1);

        if(rooms.find(roomName)==rooms.end()){
            room newRoom;
            newRoom.roomPassword = password;
            rooms[roomName] = newRoom;
        }

        thread clientThread(handleClient, clientFd, roomName, password, userName);
        clientThread.detach();
    }

    close(serverFd);
    return EXIT_SUCCESS;

}

void fillMap(){
    string userName,userPassword;

    ifstream input("user.txt");

    while(input>>userName>>userPassword){
        userCredentials[userName]=userPassword;
    }
    input.close();

    for(const auto &it : userCredentials){
        cout<<it.first<<" "<<it.second<<endl;
    }
}

bool authenticateUser(const string& userName, const string& userPassword){
    if(userCredentials.find(userName)==userCredentials.end()){
        userCredentials[userName]=userPassword;

        ofstream output("user.txt",ios::app);
        output<<userName<<' '<<userPassword<<endl;
        output.flush();
        output.close();
        return true;
    }
    auto it = userCredentials.find(userName);
    if (it->second == userPassword) {
        return true;
    }
    return false;
}

void handleClient(int clientFd, string roomName, string password, string clientName) {
    room &currentRoom = rooms[roomName];

    if (!password.empty() && currentRoom.roomPassword != password) {
        cerr << "Invalid password for room = " << roomName << endl;
        send(clientFd,"Access Denied",strlen("Access Denied"),0);
        close(clientFd);
        return;
    }
    else{
        send(clientFd,"authenticated",strlen("authenticated"),0);
    }

    currentRoom.client.push_back(clientFd);

    while (true) {
        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(clientFd, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {
            cout << "Client " << clientName << " disconnected from room " << roomName << endl;
            close(clientFd);
            currentRoom.client.erase(remove(currentRoom.client.begin(), currentRoom.client.end(), clientFd), currentRoom.client.end());
            return;
        }

        buffer[bytesReceived] = '\0';
        cout << "Client: " << clientName << " in room = " << roomName << " says: " << buffer << endl;
        
        string message = clientName + ": " + string(buffer, bytesReceived); 
        
        for (int otherClient : currentRoom.client) {
            if (otherClient != clientFd) {
                send(otherClient, message.c_str(), message.size(), 0);
            }
        }

         // Check if the client wants to exit
        if (strcmp(buffer, "#exit") == 0) {
            cout << "Client " << clientName << " requested to exit from room " << roomName << endl;
            close(clientFd);
            currentRoom.client.erase(remove(currentRoom.client.begin(), currentRoom.client.end(), clientFd), currentRoom.client.end());
            return;
        }
    }
}