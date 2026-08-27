import socket
import time
import random
import struct
import sys

BASE_PRICE = 1099.70   # 기준가
TICK_SIZE = 0.05       # 코스피200 선물 호가 단위. 가격은 이 배수로만 움직인다

def make_tick():
    t = time.strftime("%H%M%S")
    code = "A0169000"
    price = BASE_PRICE + random.randint(-40, 40) * TICK_SIZE
    qty = random.randint(1, 10)
    return f"{t}|{code}|{price:.2f}|{qty}"

def frame(data_str):
    payload = data_str.encode("ascii")  # 문자열을 바이트로 직렬화
    length = len(payload)               # 페이로드의 정확한 바이트 길이 계산
    header = struct.pack(">I", length)  # 길이를 4바이트 빅 엔디안(Big-Endian) 정수로 패킹
    return header + payload             # 헤더(4바이트) + 페이로드(N바이트) 결합

HOST = '127.0.0.1'
PORT = 9000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen()
print(f"서버가 {HOST}:{PORT} 에서 접속을 기다립니다...")

# Split 모드 여부 확인
is_split_mode = "--split" in sys.argv
if is_split_mode:
    print("[WARNING] 패킷 분할(Split) 스트리밍 모드가 활성화되었습니다.\n")

try:
    while True:
        conn, addr = s.accept()
        print(f"[{addr}] 새로운 클라이언트가 접속했습니다.")
    
        try:
            while True:
                tick_str = make_tick()
                packet = frame(tick_str)
            
                if is_split_mode:
                    cut = 2
                    conn.sendall(packet[:cut])
                    time.sleep(0.2)
                    conn.sendall(packet[cut:])
                else:
                    conn.sendall(packet)
                
                time.sleep(0.5)      

        except (ConnectionAbortedError, ConnectionResetError) as e:
            print(f"[{addr}] 클라이언트 연결 끊김 (사유: {e})")
        
        finally:
            conn.close()
            print(f"[{addr}] 소켓 자원 반환 완료. 다음 접속을 대기합니다.\n")

except KeyboardInterrupt:
    print()
    print("서버 종료")

finally:
    s.close()
