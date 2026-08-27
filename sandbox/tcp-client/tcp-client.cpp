#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result != 0) {
        std::cout << "Winsock 초기화 실패 (에러 코드: " << result << ")\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        int err = WSAGetLastError();
        std::cout << "소켓 생성 실패 (에러 코드: " << err << ")\n";

        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;

    serverAddr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    int r = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (r == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cout << "서버 접속 실패 (에러 코드: " << err << ")\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "소켓 생성 성공\n";

    closesocket(sock);
    WSACleanup();

    return 0;
}

