#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define WM_ADDSOCKET (WM_USER+1)
#define WM_RMSOCKET  (WM_USER+2)
#define SERVER_MESSAGE 1111
#define WM_SOCKET (WM_USER+3)

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	bool   isIPv6;
	char   buf[BUFSIZE];
	char   userID[20];
	int sendbytes;
	int recvbytes;
	int    state = 0;
	BOOL recvdelayed;
	SOCKETINFO* next;
};

struct COMM_MSG {
	int type;
	int size;
};

int nTotalSockets = 0;
SOCKETINFO *SocketInfoArray;
static SOCKET			 g_sockv4;
static SOCKET			 g_sockv6;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...);

void sendToAllSocket(COMM_MSG* comm_msg, char* buf);

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char *userID);
void RemoveSocketInfo(SOCKET sock);

// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
void err_display(int errcode);

// 새로 추가
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);
SOCKETINFO *GetSocketInfo(SOCKET sock);

//사용자 정의 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

static HINSTANCE     g_hInst; // 응용 프로그램 인스턴스 핸들
static HWND			 g_hDlg;
static HANDLE		 g_hServerThread;
static HWND			 g_hEditMsg;

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

SOCKETINFO* getSocketInfoByIndex(int i);

DWORD WINAPI ServerMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// 대화상자 생성
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hUserList;
	static HWND hEditMsg;
	g_hDlg = hDlg;
	LVITEM user;
	memset(&user, 0, sizeof(user));
	user.mask = LVCF_TEXT;
	user.cchTextMax = 200;


	switch (uMsg) {
	case WM_INITDIALOG:
		//컨트롤 핸들 얻기
		hEditMsg = GetDlgItem(hDlg, IDC_EDIT1);
		hUserList = GetDlgItem(hDlg, IDC_LIST1);

		// 컨트롤 초기화
		g_hServerThread = CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);
		g_hEditMsg = hEditMsg;

		return TRUE;

	case WM_SOCKET: // 소켓 관련 윈도우 메시지
		ProcessSocketMessage(hDlg, uMsg, wParam, lParam);
		return 0;


	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_BUTTON1: {
				char buf[BUFSIZ];
				int i = SendMessage(hUserList, LB_GETCURSEL, 0, 0);

				SOCKETINFO* ptr = getSocketInfoByIndex(i);
				SendMessage(hUserList, LB_DELETESTRING, (WPARAM)i, 0);	
				ptr->state = 1;
				RemoveSocketInfo(i);
				return TRUE;
			}
		}
		return DefWindowProc(hDlg, uMsg, wParam, lParam);

	case WM_ADDSOCKET: { // 유저 리스트에 추가된 것
		SendMessage(hUserList, LB_ADDSTRING, 0, (LPARAM)lParam);
		return TRUE;
	}

	case WM_RMSOCKET: {
		int index = SendMessage(hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wParam);
		SendMessage(hUserList, LB_DELETESTRING, index, 0);
		return TRUE;
		
	}
	default:
		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}
	return FALSE;
}

// 소켓 관련 윈도우 메시지 처리
void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 데이터 통신에 사용할 변수
	SOCKETINFO *ptr;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, retval;

	// 오류 발생 여부 확인
	if (WSAGETSELECTERROR(lParam)) {
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}

	// 메시지 처리
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_ACCEPT:
		addrlen = sizeof(clientaddr);
		client_sock = accept(wParam, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			return;
		}
		else {
			DisplayText("[TCPv4 서버] 클라이언트 접속: IP = %s, PORT = %d\r\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			// 소켓 정보 추가
			char userID[20];
			retval = recv(client_sock, userID, 20, 0);
			AddSocketInfo(client_sock, false, userID);
			retval = WSAAsyncSelect(client_sock, hWnd,
				WM_SOCKET, FD_READ/* | FD_WRITE */| FD_CLOSE);
			if (retval == SOCKET_ERROR) {
				err_display("WSAAsyncSelect()");
				RemoveSocketInfo(client_sock);
			}

			SendMessage(g_hDlg, WM_ADDSOCKET, (WPARAM)client_sock, (LPARAM)userID);
		}
		break;
	case FD_READ:
		COMM_MSG comm_msg;
		ptr = GetSocketInfo(wParam);
		if (ptr->recvbytes > 0) {
			ptr->recvdelayed = TRUE;
			return;
		}
		retval = recv(ptr->sock, (char*)&comm_msg, sizeof(COMM_MSG), 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			RemoveSocketInfo(ptr->sock); // 이 함수 고쳐야함
			return;
		}
		Sleep(1);
		retval = recv(ptr->sock, ptr->buf, comm_msg.size, 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			RemoveSocketInfo(ptr->sock);
			return;
		}
		ptr->recvbytes += retval;
		if (ptr->recvbytes == BUFSIZE) {
			// 받은 바이트 수 리셋
			ptr->recvbytes = 0;
		}
		sendToAllSocket(&comm_msg, ptr->buf);
		// 이 밑에 필요할랑가
		/*
	case FD_WRITE:
		ptr = GetSocketInfo(wParam);
		if (ptr->recvbytes <= ptr->sendbytes)
			return;
		// 데이터 보내기
		retval = send(ptr->sock, ptr->buf + ptr->sendbytes,
			ptr->recvbytes - ptr->sendbytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(wParam);
			return;
		}
		ptr->sendbytes += retval;
		// 받은 데이터를 모두 보냈는지 체크
		if (ptr->recvbytes == ptr->sendbytes) {
			ptr->recvbytes = ptr->sendbytes = 0;
			if (ptr->recvdelayed) {
				ptr->recvdelayed = FALSE;
				PostMessage(hWnd, WM_SOCKET, wParam, FD_READ);
			}
		}
		break;*/
	case FD_CLOSE:
		RemoveSocketInfo(wParam);
		break;
	}
}

DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

	/*-----TCP/IPv4 소켓 초기화 시작 -----*/

	// socket() listen_sockv4 -> listen_sock
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind() serveraddrv4 -> serveraddr
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");
	/*-----TCP/IPv4 소켓 초기화 끝 -----*/

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, g_hDlg,
		WM_SOCKET, FD_ACCEPT);
	if (retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");
	
	// 메시지 루프
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void sendToAllSocket(COMM_MSG* comm_msg, char* buf) { //TCP 전용

	// 현재 접속한 모든 클라이언트에게 데이터를 보냄!
	int retval;
	SOCKETINFO* ptr2 = SocketInfoArray;
	while (ptr2 != NULL){
		retval = send(ptr2->sock, (char*)comm_msg, sizeof(COMM_MSG), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			ptr2 = ptr2->next;
			RemoveSocketInfo(ptr2->sock);
			continue;
		}
		retval = send(ptr2->sock, buf, comm_msg->size, 0);

		if (retval == SOCKET_ERROR) {
			err_display("send()");
			ptr2 = ptr2->next;
			RemoveSocketInfo(ptr2->sock);
			continue;
		}
		ptr2 = ptr2->next;
	}
}
// 소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char * userID)
{
	SOCKETINFO *ptr = new SOCKETINFO;
	if (ptr == NULL) {
		DisplayText("[오류] 메모리가 부족합니다! \r\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	ptr->recvdelayed = FALSE;
	ptr->isIPv6 = isIPv6;
	strcpy(ptr->userID, userID);
	ptr->next = SocketInfoArray;
	SocketInfoArray = ptr;
	++nTotalSockets;

	return TRUE;
}

// 소켓 정보 삭제
void RemoveSocketInfo(SOCKET sock)
{
	SOCKADDR_IN clientaddr;

	SOCKETINFO *curr = SocketInfoArray;
	SOCKETINFO *prev = NULL;
	DisplayText("");
	int addrlen = sizeof(clientaddr);
	getpeername(sock, (SOCKADDR *)&clientaddr, &addrlen);

	DisplayText("[TCPv4 서버] 클라이언트 종료: IP = %s, PORT = %d \r\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	while (curr) {
		if (curr->sock == sock) {
			if (prev)
				prev->next = curr->next;
			else
				SocketInfoArray = curr->next;
			closesocket(curr->sock);
			delete curr;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	--nTotalSockets;
}

// 인덱스로 소켓 찾기
SOCKETINFO* getSocketInfoByIndex(int i) {
	SOCKETINFO* ptr = SocketInfoArray;
	for (int k = 0; k < (nTotalSockets - i - 1); k++) {
		if (ptr == NULL)
		{
			break;
		}
		ptr = ptr->next;
	}
	return ptr;
}

// 에디트 컨트롤에 문자열 출력
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditMsg);
	SendMessage(g_hEditMsg, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditMsg, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[오류] %s", (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 정보 얻기
SOCKETINFO *GetSocketInfo(SOCKET sock)
{
	SOCKETINFO *ptr = SocketInfoArray;

	while (ptr) {
		if (ptr->sock == sock)
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}