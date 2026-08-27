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

    std::cout << "서버 접속 성공!\n";

    // 1. 수신 버퍼 준비 (스택 메모리 할당)
    char buf[1024];

    // 2. 데이터 수신 무한 루프 (Blocking)
    while (true) {
        int n = recv(sock, buf, sizeof(buf), 0);

        // 3. n의 상태에 따른 엄격한 분기 처리
        if (n > 0) {
            std::cout << "[수신 바이트: " << n << "] ";
            // 주의: 그냥 cout << buf; 를 쓰면 쓰레기값이 출력됩니다. 
            // 정확히 받은 크기(n)만큼만 끊어서 출력해야 합니다.
            std::cout.write(buf, n);
            std::cout << "\n";
        }
        else if (n == 0) {
            std::cout << "\n[정상 종료] 서버가 연결을 닫았습니다.\n";
            break;
        }
        else { // n == SOCKET_ERROR
            int err = WSAGetLastError();
            std::cout << "\n[네트워크 에러] 수신 중 사고 발생 (에러 코드: " << err << ")\n";
            break;
        }
    }
    closesocket(sock);
    WSACleanup();

    return 0;
}

