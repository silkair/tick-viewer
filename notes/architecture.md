# 프로젝트 구조 — C++ 관점

> 면접 답변용이 아니라 **내가 만든 물건의 구조를 정확히 이해하기 위한** 문서다.
> 답변 대본은 `interview-qa.md`, 도메인 설명은 `tick-viewer-explainer.html` 에 있다.

**한 문장 요약**

> 다이얼로그 객체 하나를 두 스레드가 나눠 쓰되, **버퍼는 워커의 스택에 두어 아예 공유하지 않고**,
> 화면은 UI 스레드만 만지며, 둘 사이는 **메시지 큐로만** 데이터를 넘긴다.
> 공유하는 건 **소켓 핸들 하나뿐**이고 그건 종료 신호용이다.

---

## 1. 클래스는 둘뿐이다

```
ChtsviewerApp : CWinApp          <- 프로그램 자체
    └ InitInstance()              <- 시작점. 창을 만들고 띄운다

ChtsviewerDlg : CDialogEx         <- 창 하나
    ├ m_tickList : CListCtrl      <- 목록 컨트롤
    ├ m_sock     : SOCKET         <- 소켓 핸들
    └ m_hIcon    : HICON
```

`ChtsviewerApp` 은 전역 객체 **`theApp` 하나**로 존재한다 (`hts-viewer.cpp`).
MFC에서는 `main` 이 감춰져 있고, **`theApp` 이 만들어지는 것 자체가 프로그램의 시작**이다.

`ChtsviewerDlg` 객체도 딱 하나다. 그래서:

> **객체는 하나, 스레드는 둘.**

두 스레드가 **같은 다이얼로그 객체 하나**를 함께 쓴다.
`ReceiveThreadProc` 의 `pDlg` 도 UI 스레드가 만든 바로 그 객체다.

---

## 2. 어느 함수가 어느 스레드에서 도는가

| 함수 | 스레드 | 하는 일 |
| --- | --- | --- |
| `InitInstance()` | **UI** | 창 생성 |
| `OnInitDialog()` | **UI** | 열 만들기 + **워커 스레드 시작** |
| `ReceiveThreadProc()` | **워커** | 진입점. `this` 복원 |
| `ConnectAndReceive()` | **워커** | 접속 · `recv` · 파싱 · `PostMessage` |
| `OnTickReceived()` | **UI** | 메시지 수신 · `delete` |
| `AddTickFromBytes()` | **UI** | `|` 로 쪼개기 · `CString` 변환 |
| `AddTick()` | **UI** | 목록에 행 추가 |
| `OnDestroy()` | **UI** | `closesocket` 으로 워커 깨우기 |
| `OnPaint()` 등 | **UI** | MFC가 알아서 |

### 헷갈리기 쉬운 점

**`AddTick` 은 `ChtsviewerDlg` 의 멤버 함수지만 워커 스레드에서는 절대 안 불린다.**

> **어느 클래스에 속하느냐 ≠ 어느 스레드에서 도느냐**

C++ 문법상으로는 워커에서 `pDlg->AddTick(...)` 을 부를 수 있고 **컴파일도 된다.**
막아주는 게 없다. 3-c 단계에서 실제로 그렇게 했었다.

**"이 함수는 UI 스레드에서만" 은 문법이 아니라 약속이다.** 지키는 건 사람 몫이다.
Java의 `synchronized` 처럼 언어가 돕는 장치가 없고, Android가 `runOnUiThread` 를 안 쓰면
예외를 던지는 것과도 다르다. **Win32는 그냥 조용히 이상하게 동작한다.**

---

## 3. 변수가 어디 있는가 — 설계의 핵심

```cpp
void ChtsviewerDlg::ConnectAndReceive()   // 워커 스레드에서 실행
{
    char accum[4096];      // <- 지역 변수 = 워커 스레드의 스택
    int  accumLen = 0;
    ...
}
```

| | 어디에 | 누가 접근 가능 |
| --- | --- | --- |
| `accum`, `accumLen` | **워커 스레드의 스택** | **워커만** |
| `m_sock` | 다이얼로그 객체 안 | **두 스레드 다** |
| `m_tickList` | 다이얼로그 객체 안 | **UI만 (약속)** |

**스레드마다 스택이 따로 있다.** `accum` 은 워커 스레드가 시작될 때 그 스레드의 스택에
잡히고, UI 스레드는 그 주소를 알 방법조차 없다.

> **공유하지 않는 것은 경쟁 조건이 생길 수 없다.**

뮤텍스 같은 동기화 장치를 하나도 안 쓰고도 안전한 이유가 이것이다. **애초에 안 나눠 가졌다.**

`accum` 을 멤버 변수로 뒀다면 두 스레드가 공유하게 되어 보호가 필요했을 것이다.
**지역 변수로 둔 것이 곧 설계 결정이다.**

---

## 4. 공유하는 것은 딱 하나 — `m_sock`

```
워커 스레드:  recv(m_sock, ...)          <- 읽는다
UI 스레드:    closesocket(m_sock)        <- 쓴다 (OnDestroy)
```

**유일한 진짜 공유 지점**이고 **의도된 것**이다. 워커를 깨우려면 UI가 그 소켓을
건드려야만 하기 때문이다.

### 정직하게 — 여기엔 동기화가 없다

워커가 `recv` 를 부르기 직전에 UI가 소켓을 닫으면, 워커는 이미 닫힌 핸들로 `recv` 를 부른다.
실제로는 에러가 나면서 루프를 빠져나가니 문제가 안 되지만 **엄밀히는 경쟁 조건이다.**

물어보면:

> `m_sock` 하나만 두 스레드가 공유하는데 동기화 장치는 두지 않았습니다. 닫힌 핸들로
> `recv` 를 불러도 에러가 나면서 정상적으로 빠져나가기 때문인데, **엄밀히는 경쟁 조건이
> 맞습니다.** 더 다듬는다면 여기에 보호를 두거나 이벤트 객체를 쓰는 방식이 있을 것 같습니다.

---

## 5. 데이터의 여정 — 타입이 네 번 바뀐다

```
① 소켓                              워커 스레드
      ↓  recv()
   char accum[4096]                 워커의 스택. 1바이트 문자
      ↓  new std::string(accum+4, bodyLen)
② std::string*                      힙. 여기서 소유권 발생
      ↓  PostMessage((WPARAM)pBody)
   ───────── 스레드 경계 ─────────
      ↓  OnTickReceived
③ std::string*                      UI 스레드가 받음 → delete
      ↓  substr 로 '|' 쪼개기
   std::string × 4                   UI의 스택. 아직 1바이트 문자
      ↓  CString(time.c_str())
④ CString × 4                        2바이트 문자로 변환
      ↓  InsertItem / SetItemText
   CListCtrl 내부                     화면
```

| 단계 | 왜 바꾸나 |
| --- | --- |
| ① → ② | 워커의 `accum` 은 곧 `memmove` 로 덮인다. **힙에 복사해야 살아남는다** |
| ② → ③ | `PostMessage` 는 정수 하나만 실을 수 있어서 **포인터를 정수로 캐스팅** |
| ③ → ④ | MFC는 유니코드 빌드라 **2바이트 문자**를 요구한다 |

`WPARAM` 은 그냥 정수 타입이다. `std::string*` 을 `(WPARAM)` 으로 캐스팅해 싣고 받는 쪽에서
`(std::string*)` 로 되돌린다. **`void*` 로 `this` 를 실어 나른 것과 같은 수법이다.**

---

## 6. 시작부터 끝까지

```
1.  theApp 이 만들어짐  →  InitInstance()
2.  ChtsviewerDlg 객체 생성  →  DoModal()
3.  OnInitDialog()
      ├ 열 4개 만들기
      └ AfxBeginThread(ReceiveThreadProc, this)   <- 여기서 스레드가 갈라짐
                                    │
        ┌───────────────────────────┴──── 워커 스레드 시작
        │
4.  UI 스레드                    5.  워커 스레드
    창이 뜨고 메시지 루프 돌기       WSAStartup → socket → connect
    (클릭, 그리기, 메시지 처리)      while(true) { recv → 파싱 → PostMessage }
        │                                       │
        └────────── PostMessage ────────────────┘
6.  OnTickReceived() 가 UI 스레드에서 불림
      → AddTickFromBytes → AddTick → 화면

7.  사용자가 X 클릭
      → OnDestroy() [UI]  closesocket(m_sock)
                                    ↓
      → 워커의 recv 가 에러로 깨어남 → break → ConnectAndReceive 종료
      → 워커 스레드 종료
```

---

## 7. 알려진 구멍

정직하게 적어둔다. **"다시 만든다면?" 질문의 재료다.**

### ① UI 스레드가 워커의 종료를 기다리지 않는다

`OnDestroy` 는 소켓만 닫고 바로 돌아간다. 그 시점에 워커는 아직 `ConnectAndReceive` 안에
있을 수 있고, 이론적으로는 **창이 파괴된 뒤에 `PostMessage` 를 부를 수 있다.**

실제로는 `PostMessage` 가 무효한 창에 대해 **실패만 하고 크래시는 안 나서** 드러나지 않고,
프로세스도 곧 끝난다. 더 엄밀히 하려면 `AfxBeginThread` 가 돌려주는 `CWinThread*` 를
보관했다가 `WaitForSingleObject` 로 종료를 기다려야 한다.

### ② 예외가 나면 `delete` 를 건너뛴다

```cpp
AddTickFromBytes(pBody->data(), (int)pBody->size());
delete pBody;      // 위에서 예외가 나면 도달 못 함
```

### ③ 창이 닫힐 때 큐에 남은 메시지는 처리되지 않는다

거기 실린 `std::string` 객체들은 아무도 지우지 않는다. 프로세스가 곧 끝나니 실질적
피해는 없지만 구조적으로는 누수 경로다.

### ④ 재접속이 없다

연결이 끊기면 워커 스레드가 종료되고 그걸로 끝이다. 프로그램을 다시 켜야 한다.

### ⑤ `PostMessage` 큐에 상한이 없다

UI가 느리면 메시지가 계속 쌓인다. 초당 두 건이라 문제가 없을 뿐, 부하가 커지면
큐 길이를 보고 오래된 건을 버리는 처리가 필요하다.
