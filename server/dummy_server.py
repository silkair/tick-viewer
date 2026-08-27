"""
더미 시세 서버 — HTS '시간대별 체결' 클라이언트 연습용

가짜 체결 데이터를 TCP로 계속 흘려보낸다.
한 건은 [길이 4바이트 big-endian][본문] 형식이다. 자세한 규칙은 PROTOCOL.md 참고.

실행:
    py dummy_server.py           # 기본  — 0.5초마다 한 건씩
    py dummy_server.py --burst   # 3건을 한 번의 sendall 로 붙여서 전송
    py dummy_server.py --split   # 한 건을 두 조각으로 쪼개서 전송

--burst 와 --split 은 C++ 클라이언트의 파서를 괴롭히기 위한 모드다.
"""

import socket
import time
import random
import struct
import sys

HOST = '127.0.0.1'
PORT = 9000

SYMBOL = "A0169000"
BASE_PRICE = 1099.70   # 기준가
TICK_SIZE = 0.05       # 코스피200 선물 호가 단위. 가격은 이 배수로만 움직인다


def make_tick():
    t = time.strftime("%H%M%S")
    price = BASE_PRICE + random.randint(-40, 40) * TICK_SIZE
    qty = random.choice([1, 1, 1, 2, 2, 3, 5, 10])   # 실제 체결은 소량 주문이 훨씬 많다
    return f"{t}|{SYMBOL}|{price:.2f}|{qty}"


def frame(data_str):
    payload = data_str.encode("ascii")  # 문자열을 바이트로 직렬화
    length = len(payload)               # 페이로드의 정확한 바이트 길이 계산
    header = struct.pack(">I", length)  # 길이를 4바이트 빅 엔디안(Big-Endian) 정수로 패킹
    return header + payload             # 헤더(4바이트) + 페이로드(N바이트) 결합


def read_mode():
    """명령행 인자에서 전송 모드를 읽는다."""
    if "--burst" in sys.argv:
        return "burst"
    if "--split" in sys.argv:
        return "split"
    return "normal"


def send_ticks(conn, mode):
    """한 클라이언트에게 연결이 끊길 때까지 체결 데이터를 계속 보낸다."""
    while True:
        if mode == "burst":
            # 3건을 하나의 sendall 로 붙여서 보낸다.
            # 클라이언트의 recv 한 번에 여러 건이 뭉쳐 도착한다.
            ticks = [make_tick() for _ in range(3)]
            conn.sendall(b"".join(frame(t) for t in ticks))
            for t in ticks:
                print(f"[전송] {t}  (붙임)")

        elif mode == "split":
            # 길이 프리픽스 4바이트 도중에 끊어서 보낸다.
            # 클라이언트는 길이조차 다 못 읽은 상태를 겪게 된다.
            tick_str = make_tick()
            packet = frame(tick_str)
            cut = 2
            conn.sendall(packet[:cut])
            time.sleep(0.2)
            conn.sendall(packet[cut:])
            print(f"[전송] {tick_str}  (쪼갬 {cut}/{len(packet) - cut})")

        else:
            tick_str = make_tick()
            conn.sendall(frame(tick_str))
            print(f"[전송] {tick_str}")

        time.sleep(0.5)


def main():
    mode = read_mode()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        s.settimeout(1.0)   # accept() 가 1초마다 돌아오게 해서 Ctrl+C 를 처리할 틈을 준다
        print(f"서버가 {HOST}:{PORT} 에서 접속을 기다립니다... (mode={mode})")
        print("종료하려면 Ctrl+C")
        print()

        while True:
            try:
                conn, addr = s.accept()
            except TimeoutError:
                continue   # 1초 동안 아무도 안 붙었다. 다시 기다린다

            print(f"[{addr}] 새로운 클라이언트가 접속했습니다.")

            try:
                send_ticks(conn, mode)

            except (ConnectionAbortedError, ConnectionResetError) as e:
                print(f"[{addr}] 클라이언트 연결 끊김 (사유: {e})")

            finally:
                conn.close()
                print(f"[{addr}] 소켓 자원 반환 완료. 다음 접속을 대기합니다.")
                print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        print("서버 종료")
