import socket
import time
import random

# 체결 데이터 생성 함수
def make_tick():
    t = time.strftime("%H%M%S")
    code = "A0169000"
    price = random.uniform(1094.70, 1104.70)
    qty = random.randint(1, 10)
    return f"{t}|{code}|{price:.2f}|{qty}"

HOST = '127.0.0.1'
PORT = 9000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.bind((HOST, PORT))

s.listen()
print(f"서버가 {HOST}:{PORT} 에서 접속을 기다립니다...")

while True:
    conn, addr = s.accept()
    print(f"[{addr}] 새로운 클라이언트가 접속했습니다.")
    
    try:
        while True:
            tick_str = make_tick()
            conn.sendall(tick_str.encode("ascii"))
            time.sleep(0.5)
            
    except (ConnectionAbortedError, ConnectionResetError) as e:
        print(f"[{addr}] 클라이언트 연결 끊김 (사유: {e})")
        
    finally:
        conn.close()
        print(f"[{addr}] 소켓 자원 반환 완료. 다음 접속을 대기합니다.\n")