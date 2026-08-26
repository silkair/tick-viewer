"""
더미 시세 서버 — HTS 클라이언트 연습용

가짜 체결 데이터를 TCP로 계속 흘려보낸다.
C++ 클라이언트가 붙어서 받아 화면 목록에 뿌리는 것이 목표.

실행:
    py tick_server.py            # 기본: 0.5초마다 한 건씩
    py tick_server.py --burst    # 3건을 한 번에 붙여서 전송
    py tick_server.py --split    # 한 건을 두 조각으로 쪼개서 전송

--burst 와 --split 은 클라이언트 파서를 괴롭히기 위한 모드다.
자세한 이유는 같은 폴더의 PROTOCOL.md 참고.
"""

import socket
import struct
import time
import random
import sys

HOST = "127.0.0.1"
PORT = 9000

# 화면에서 본 코스피200 선물 202609 종목코드와 가격대를 흉내 냈다
SYMBOL = "A0169000"
BASE_PRICE = 1099.70


def make_tick() -> str:
    """체결 한 건을 '시간|종목코드|가격|수량' 문자열로 만든다."""
    hhmmss = time.strftime("%H%M%S")
    price = BASE_PRICE + random.randint(-40, 40) * 0.05
    qty = random.choice([1, 1, 1, 2, 2, 3, 5, 10])
    return f"{hhmmss}|{SYMBOL}|{price:.2f}|{qty}"


def frame(body: str) -> bytes:
    """
    메시지 앞에 길이를 4바이트로 붙인다.

    '>I' 는 big-endian(네트워크 바이트 순서) 부호 없는 4바이트 정수.
    C++ 쪽에서는 ntohl() 로 되돌린다.

        [ 길이 4바이트 ][ 본문 ]
        [ 0x00000018  ][ 152233|A0169000|1099.70|3 ]
    """
    data = body.encode("ascii")
    return struct.pack(">I", len(data)) + data


def serve_forever(mode: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((HOST, PORT))
        srv.listen(1)
        print(f"[server] 대기 중  {HOST}:{PORT}  (mode={mode})")
        print("[server] 종료하려면 Ctrl+C")

        while True:
            conn, addr = srv.accept()
            print(f"[server] 접속됨  {addr}")
            try:
                while True:
                    if mode == "burst":
                        # 3건을 하나의 sendall 로 붙여서 보낸다.
                        # 클라이언트의 recv 한 번에 여러 건이 뭉쳐 도착한다.
                        bodies = [make_tick() for _ in range(3)]
                        conn.sendall(b"".join(frame(b) for b in bodies))
                        for b in bodies:
                            print(f"[server] 보냄(붙임)  {b}")

                    elif mode == "split":
                        # 한 건을 두 조각으로 쪼개서 보낸다.
                        # 클라이언트의 recv 한 번에 메시지가 반만 도착한다.
                        body = make_tick()
                        packet = frame(body)
                        cut = len(packet) // 2
                        conn.sendall(packet[:cut])
                        time.sleep(0.2)
                        conn.sendall(packet[cut:])
                        print(f"[server] 보냄(쪼갬)  {body}")

                    else:
                        body = make_tick()
                        conn.sendall(frame(body))
                        print(f"[server] 보냄  {body}")

                    time.sleep(0.5)

            except (ConnectionResetError, BrokenPipeError):
                print("[server] 연결 끊김")
            finally:
                conn.close()


if __name__ == "__main__":
    mode = "normal"
    if "--burst" in sys.argv:
        mode = "burst"
    elif "--split" in sys.argv:
        mode = "split"

    try:
        serve_forever(mode)
    except KeyboardInterrupt:
        print("\n[server] 종료")