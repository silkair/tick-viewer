
// hts-viewerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "hts-viewer.h"
#include "hts-viewerDlg.h"
#include "afxdialogex.h"
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr int MAX_ROWS = 500;

LRESULT ChtsviewerDlg::OnTickReceived(WPARAM wParam, LPARAM lParam)
{
	std::string* pBody = (std::string*)wParam;

	// 안전한 UI 스레드 컨텍스트에서 UI 갱신 함수 호출
	AddTickFromBytes(pBody->data(), (int)pBody->size());

	// 워커 스레드가 할당한 메모리의 소유권을 넘겨받아 해제 (Memory Leak 방지)
	delete pBody;

	return 0;
}

void ChtsviewerDlg::ConnectAndReceive()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == INVALID_SOCKET) { WSACleanup(); return; }

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9000);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (connect(m_sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET; // 초기화 습관화
		WSACleanup();
		return;
	}

	char accum[4096];
	int accumLen = 0;

	while (true) {
		int n = recv(m_sock, accum + accumLen, sizeof(accum) - accumLen, 0);

		if (n > 0) {
			accumLen += n;

			while (accumLen >= 4) {
				unsigned int netLen;
				memcpy(&netLen, accum, 4);
				unsigned int bodyLen = ntohl(netLen);

				if (bodyLen == 0 || bodyLen > sizeof(accum) - 4) {
					if (m_sock != INVALID_SOCKET) {
						closesocket(m_sock);
						m_sock = INVALID_SOCKET;
					}
					break;
				}

				int used = 4 + bodyLen;
				if (accumLen < used) {
					break;
				}
				std::string* pBody = new std::string(accum + 4, bodyLen);

				PostMessage(WM_TICK_RECEIVED, (WPARAM)pBody, 0);

				memmove(accum, accum + used, accumLen - used);
				accumLen -= used;
			}
		}
		else {
			break;
		}
	}

	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	WSACleanup();
}

UINT ChtsviewerDlg::ReceiveThreadProc(LPVOID pParam)
{
	// 1. void* 로 넘어온 this 포인터를 원래 다이얼로그 타입으로 복원
	ChtsviewerDlg* pDlg = (ChtsviewerDlg*)pParam;

	// 2. 복원된 인스턴스를 통해 네트워크 수신 루프 실행
	pDlg->ConnectAndReceive();

	return 0;
}

void ChtsviewerDlg::AddTickFromBytes(const char* body, int len)
{
	// 1. 길이를 명시하여 널(Null) 종료 문자의 위협으로부터 안전하게 캡슐화
	std::string s(body, len);

	// 2. 파이프(|) 구분자 기반 파싱 (포인터 인덱싱 계산)
	size_t p1 = s.find('|');
	std::string time = s.substr(0, p1);

	size_t p2 = s.find('|', p1 + 1);
	std::string code = s.substr(p1 + 1, p2 - p1 - 1);

	size_t p3 = s.find('|', p2 + 1);
	std::string price = s.substr(p2 + 1, p3 - p2 - 1);

	if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) return;

	std::string qty = s.substr(p3 + 1);

	// 3. ASCII 문자열 -> OS 기본 코드페이지 경유 -> MFC 유니코드(CString) 변환 후 렌더링
	AddTick(CString(time.c_str()), CString(code.c_str()), CString(price.c_str()), CString(qty.c_str()));
}

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// ChtsviewerDlg 대화 상자

ChtsviewerDlg::ChtsviewerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HTSVIEWER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ChtsviewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TICK_LIST, m_tickList);
	DDX_Control(pDX, IDC_TICK_LIST, m_tickList);
	DDX_Control(pDX, IDC_TICK_LIST, m_tickList);
}

BEGIN_MESSAGE_MAP(ChtsviewerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_TICK_RECEIVED, &ChtsviewerDlg::OnTickReceived)
//	ON_WM_DESTROYCLIPBOARD()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// ChtsviewerDlg 메시지 처리기

BOOL ChtsviewerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	m_tickList.InsertColumn(0, _T("시각"), LVCFMT_LEFT, 80);
	m_tickList.InsertColumn(1, _T("종목코드"), LVCFMT_LEFT, 100);
	m_tickList.InsertColumn(2, _T("가격"), LVCFMT_RIGHT, 90);
	m_tickList.InsertColumn(3, _T("수량"), LVCFMT_RIGHT, 60);
	m_tickList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	AfxBeginThread(ReceiveThreadProc, this); // 백그라운드 워커 스레드로 분리 실행

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void ChtsviewerDlg::AddTick(const CString& time, const CString& code, const CString& price, const CString& qty)
{
	int row = m_tickList.InsertItem(0, time);
	m_tickList.SetItemText(row, 1, code);
	m_tickList.SetItemText(row, 2, price);
	m_tickList.SetItemText(row, 3, qty);

	if (m_tickList.GetItemCount() > MAX_ROWS) {
		m_tickList.DeleteItem(m_tickList.GetItemCount() - 1);
	}
}

void ChtsviewerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void ChtsviewerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR ChtsviewerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void ChtsviewerDlg::OnDestroy()
{
	// 워커 스레드의 recv()를 강제로 깨우기 위해 소켓을 닫아버림
	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	CDialogEx::OnDestroy();
}
