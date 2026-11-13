
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace std;

#define PORT 8080

struct sockaddr_in server;

int main()
{
    WSADATA wsaData;

    int nStatus = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (nStatus != 0)
    {
        cout << "WSAStartup() failed: " << nStatus << endl;
        return 1;
    }

    SOCKET nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (nSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    cout << "Client socket created successfully!" << endl;

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    memset(server.sin_zero, 0, sizeof(server.sin_zero));

    nStatus = connect(nSocket, (struct sockaddr*)&server, sizeof(server));

    if (nStatus == SOCKET_ERROR)
    {
        cout << "connect() failed: " << WSAGetLastError() << endl;
        closesocket(nSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server successfully!" << endl;

    while (true)
    {
        string message;

        cout << "Please enter your message: ";
        getline(cin, message);

        if (message == "exit")
            break;

        int totalSent = 0;

        while (totalSent < message.length())
        {
            nStatus = send(
                nSocket,
                message.c_str() + totalSent,
                static_cast<int>(message.length() - totalSent),
                0
            );

            if (nStatus == SOCKET_ERROR)
            {
                cout << "send() failed: " << WSAGetLastError() << endl;
                closesocket(nSocket);
                WSACleanup();
                return 1;
            }

            totalSent += nStatus;
        }

        cout << "Message sent successfully!" << endl;
    }

    closesocket(nSocket);
    WSACleanup();

    cout << "Client disconnected." << endl;

    return 0;
}
