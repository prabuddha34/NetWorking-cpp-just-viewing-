#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <iostream>
#define PORT 8080

int main() {
    WSADATA wsaData;
    int iVal;
    iVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iVal != 0) {
        std::cout << "WSAStartup failed with error " << iVal << std::endl;
        exit(EXIT_FAILURE);
    }

    // We have to make a socket object that will hear the client!
    SOCKET listenSocket = INVALID_SOCKET;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    // check for errors of the listenSocket!
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // we have to bind the socket!
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    iVal = bind(listenSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));
    if (iVal == SOCKET_ERROR) {
        std::cout << "Bind failed with error " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed with error " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    std::cout << "Listening on port " << PORT << "..." << std::endl;

    // next step: accept() goes here we have to accept something !
    
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}