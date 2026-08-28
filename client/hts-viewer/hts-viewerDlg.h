
// hts-viewerDlg.h: 헤더 파일
//

#pragma once
#define WM_TICK_RECEIVED (WM_USER + 1)

// ChtsviewerDlg 대화 상자
class ChtsviewerDlg : public CDialogEx
{
// 생성입니다.
public:
	ChtsviewerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HTSVIEWER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTickReceived(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	void AddTick(const CString& time, const CString& code, const CString& price, const CString& qty);
	void AddTickFromBytes(const char* body, int len);
	void ConnectAndReceive();
	CListCtrl m_tickList;

	static UINT ReceiveThreadProc(LPVOID pParam);
	SOCKET m_sock = INVALID_SOCKET;
//	afx_msg void OnDestroyClipboard();
	afx_msg void OnDestroy();
};
