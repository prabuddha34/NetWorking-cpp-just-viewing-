
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstring>

using namespace std;

#define PORT 8080

struct sockaddr_in srv;

int main()
{
    WSADATA ws;

    int nStatus = WSAStartup(MAKEWORD(2, 2), &ws);

    if (nStatus != 0)
    {
        cout << "WSAStartup failed: " << nStatus << endl;
        return 1;
    }

    cout << "Winsock initialized successfully!" << endl;

    // Stream socket: Connection-oriented TCP
    // Datagram socket: Connectionless UDP

    SOCKET nListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (nListener == INVALID_SOCKET)
    {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    cout << "Socket created successfully!" << endl;

    // Configure server address
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);

    memset(srv.sin_zero, 0, sizeof(srv.sin_zero));

    // Bind socket to port
    nStatus = bind(nListener, (struct sockaddr*)&srv, sizeof(srv));

    if (nStatus == SOCKET_ERROR)
    {
        cout << "Bind failed: " << WSAGetLastError() << endl;

        closesocket(nListener);
        WSACleanup();
        return 1;
    }

    cout << "Socket bound to port " << PORT << endl;

    // Start listening
    nStatus = listen(nListener, 5);

    if (nStatus == SOCKET_ERROR)
    {
        cout << "Listen failed: " << WSAGetLastError() << endl;

        closesocket(nListener);
        WSACleanup();
        return 1;
    }

    cout << "Server is listening!" << endl;
    cout << "Waiting for a client connection..." << endl;

    // Accept client connection
    SOCKET nClientSocket = accept(nListener, NULL, NULL);

    if (nClientSocket == INVALID_SOCKET)
    {
        cout << "Accept failed: " << WSAGetLastError() << endl;

        closesocket(nListener);
        WSACleanup();
        return 1;
    }

    cout << "\nClient connected successfully!" << endl;

    char sBuff[1024];

    // Receive messages from client
    while (true)
    {
        memset(sBuff, 0, sizeof(sBuff));

        nStatus = recv(nClientSocket, sBuff, sizeof(sBuff) - 1, 0);

        if (nStatus > 0)
        {
            sBuff[nStatus] = '\0';

            cout << "Received (" << nStatus << " bytes): "
                 << sBuff << endl;
        }
        else if (nStatus == 0)
        {
            cout << "\nClient disconnected." << endl;
            break;
        }
        else
        {
            cout << "Receive failed: " << WSAGetLastError() << endl;
            break;
        }
    }

    // Cleanup
    closesocket(nClientSocket);
    closesocket(nListener);

    WSACleanup();

    cout << "Server stopped successfully." << endl;

    return 0;
}
