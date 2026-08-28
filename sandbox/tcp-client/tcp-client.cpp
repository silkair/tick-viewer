#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "Winsock 초기화 실패\n";
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

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();   // 정리하기 전에 이유부터 확보한다
        std::cout << "서버 접속 실패 (에러 코드: " << err << ")\n";
        closesocket(sock);             // 확보한 뒤에 정리한다
        WSACleanup();
        return 1;
    }

    std::cout << "서버 접속 성공!\n";

    char buf[1024];
    char accum[4096];
    int accumLen = 0;

    while (true) {
        int n = recv(sock, buf, sizeof(buf), 0);

        if (n > 0) {
            if (accumLen + n > (int)sizeof(accum)) {
                std::cout << "누적 버퍼가 넘쳤습니다\n";
                break;
            }

            memcpy(accum + accumLen, buf, n);
            accumLen += n;

            std::cout << "[받음 " << n << "] [누적 " << accumLen << "]\n";

            while (accumLen >= 4) {
                unsigned int netLen;
                memcpy(&netLen, accum, 4);
                unsigned int bodyLen = ntohl(netLen);

                if (bodyLen == 0 || bodyLen > sizeof(accum) - 4) {
                    std::cout << "  -> [프로토콜 위반] 길이가 이상합니다: " << bodyLen << "\n";
                    break;   // 이 연결은 신뢰할 수 없음
                }

                int used = 4 + bodyLen;

                // 본문이 아직 다 도착하지 않았다면 더 이상 꺼낼 수 없으므로 탈출(break)
                if (accumLen < used) {
                    std::cout << "  -> [대기] 본문 미완성\n";
                    break;
                }

                // 데이터 1건 추출
                std::cout << "  -> [완성] ";
                std::cout.write(accum + 4, bodyLen);
                std::cout << "\n";

                // 버퍼 당기기
                memmove(accum, accum + used, accumLen - used);
                accumLen -= used;
            }

            // 2. 루프를 빠져나왔을 때 남은 찌꺼기가 4바이트 미만인 경우
            if (accumLen > 0 && accumLen < 4) {
                std::cout << "  -> [대기] 4바이트 미만 (길이 파악 불가)\n";
            }
        }
        else if (n == 0) {
            std::cout << "\n[정상 종료] 서버가 연결을 닫았습니다.\n";
            break;
        }
        else {
            int err = WSAGetLastError();
            std::cout << "\n[네트워크 에러] 수신 중 사고 발생 (에러 코드: " << err << ")\n";
            break;
        }
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}