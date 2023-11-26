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

// ���� ���� ������ ���� ����ü�� ����
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

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char* fmt, ...);

void sendToAllSocket(COMM_MSG* comm_msg, char* buf);

// ���� ���� �Լ�
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char *userID);
void RemoveSocketInfo(SOCKET sock);

// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
void err_display(int errcode);

// ���� �߰�
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);
SOCKETINFO *GetSocketInfo(SOCKET sock);

//����� ���� �Լ�
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

static HINSTANCE     g_hInst; // ���� ���α׷� �ν��Ͻ� �ڵ�
static HWND			 g_hDlg;
static HANDLE		 g_hServerThread;
static HWND			 g_hEditMsg;

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

SOCKETINFO* getSocketInfoByIndex(int i);

DWORD WINAPI ServerMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// ��ȭ���� ����
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
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
		//��Ʈ�� �ڵ� ���
		hEditMsg = GetDlgItem(hDlg, IDC_EDIT1);
		hUserList = GetDlgItem(hDlg, IDC_LIST1);

		// ��Ʈ�� �ʱ�ȭ
		g_hServerThread = CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);
		g_hEditMsg = hEditMsg;

		return TRUE;

	case WM_SOCKET: // ���� ���� ������ �޽���
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

	case WM_ADDSOCKET: { // ���� ����Ʈ�� �߰��� ��
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

// ���� ���� ������ �޽��� ó��
void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// ������ ��ſ� ����� ����
	SOCKETINFO *ptr;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, retval;

	// ���� �߻� ���� Ȯ��
	if (WSAGETSELECTERROR(lParam)) {
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}

	// �޽��� ó��
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_ACCEPT:
		addrlen = sizeof(clientaddr);
		client_sock = accept(wParam, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			return;
		}
		else {
			DisplayText("[TCPv4 ����] Ŭ���̾�Ʈ ����: IP = %s, PORT = %d\r\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			// ���� ���� �߰�
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
			RemoveSocketInfo(ptr->sock); // �� �Լ� ���ľ���
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
			// ���� ����Ʈ �� ����
			ptr->recvbytes = 0;
		}
		sendToAllSocket(&comm_msg, ptr->buf);
		// �� �ؿ� �ʿ��Ҷ���
		/*
	case FD_WRITE:
		ptr = GetSocketInfo(wParam);
		if (ptr->recvbytes <= ptr->sendbytes)
			return;
		// ������ ������
		retval = send(ptr->sock, ptr->buf + ptr->sendbytes,
			ptr->recvbytes - ptr->sendbytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(wParam);
			return;
		}
		ptr->sendbytes += retval;
		// ���� �����͸� ��� ���´��� üũ
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

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

	/*-----TCP/IPv4 ���� �ʱ�ȭ ���� -----*/

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
	/*-----TCP/IPv4 ���� �ʱ�ȭ �� -----*/

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, g_hDlg,
		WM_SOCKET, FD_ACCEPT);
	if (retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");
	
	// �޽��� ����
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void sendToAllSocket(COMM_MSG* comm_msg, char* buf) { //TCP ����

	// ���� ������ ��� Ŭ���̾�Ʈ���� �����͸� ����!
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
// ���� ���� �߰�
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char * userID)
{
	SOCKETINFO *ptr = new SOCKETINFO;
	if (ptr == NULL) {
		DisplayText("[����] �޸𸮰� �����մϴ�! \r\n");
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

// ���� ���� ����
void RemoveSocketInfo(SOCKET sock)
{
	SOCKADDR_IN clientaddr;

	SOCKETINFO *curr = SocketInfoArray;
	SOCKETINFO *prev = NULL;
	DisplayText("");
	int addrlen = sizeof(clientaddr);
	getpeername(sock, (SOCKADDR *)&clientaddr, &addrlen);

	DisplayText("[TCPv4 ����] Ŭ���̾�Ʈ ����: IP = %s, PORT = %d \r\n",
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

// �ε����� ���� ã��
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

// ����Ʈ ��Ʈ�ѿ� ���ڿ� ���
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

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� �Լ� ���� ���
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[����] %s", (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ���� ���� ���
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