#pragma comment(lib, "ws2_32")
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <Windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <Commctrl.h>
#include "resource.h"
#include <winresrc.h>
#include <math.h>
#include <cmath>
#include "PrjClient.h"
#include <string.h>
#include <cstring>
#include <tchar.h>
//#include "pch.h"
//#include "tipsware.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable:4996)


/* ������ ���� ���� ���� */
static HINSTANCE     g_hInst; // ���� ���α׷� �ν��Ͻ� �ڵ�
static HWND          g_hDrawWnd; // �׸��� �׸� ������
static HWND          g_hButtonSendMsg; // '�޽��� ����' ��ư
static HWND          g_hEditStatus; // ���� �޽��� ���
static HWND          g_hBtnErasePic;  // [�׸� �����] ��ư
static HWND          g_hBtnSendFile;  // [���� ����] ��ư
static HWND			 g_hButtonClose;
static HWND			 g_hButtonConnect;
static HWND			 g_hButtonIsIPv6;
static HWND			 g_hEditIPaddr;
static HWND			 g_hEditPort;
static HWND			 g_hEditUserID;
//static HWND			 g_hBtnErasePic;
static HWND			 g_hButtonNotice; //[���� ���] ��ư //����
static HWND			 g_hEditNotice; // ���� ���� ��� //����


/* ��� ���� ���� ���� */
static char          g_ipaddr[64]; // ���� IP �ּ�
static u_short       g_port; // ���� ��Ʈ ��ȣ
static BOOL          g_isIPv6; // IPv4 or IPv6 �ּ�?
static HANDLE        g_hClientThread; // ������ �ڵ�
static volatile BOOL g_bStart; // ��� ���� ����
static SOCKET        g_sock; // Ŭ���̾�Ʈ ����
static HANDLE        g_hReadEvent, g_hWriteEvent; // �̺�Ʈ �ڵ�
static struct sockaddr_in g_serveraddr;
static struct sockaddr_in6 g_serveraddr6;


/* �޽��� ���� ���� ���� */
static CHAT_MSG      g_chatmsg; // ä�� �޽��� ����
static NOTICE_MSG      g_noticemsg; // ���� �޽��� ���� //����

static DRAWLINE_MSG  g_drawmsg; // �� �׸��� �޽��� ����
static int           g_drawcolor; // �� �׸��� ����
static ERASEPIC_MSG  g_erasepicmsg;


static COMM_MSG		 g_drawsize;

int chat;  //����


int recvnUDP(SOCKET s, char* buf, int len, int flags, sockaddr* sockaddr, int* addrlen);

// ���� �Լ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

	// �̺�Ʈ ����
	g_hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(g_hReadEvent == NULL) return 1;
	g_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(g_hWriteEvent == NULL) return 1;

	// ���� �ʱ�ȭ(�Ϻ�)
	g_chatmsg.type = CHATTING;
	g_noticemsg.type = NOTICE; //����
	g_drawmsg.type = DRAWLINE;
	g_drawmsg.color = Color;
	g_drawmsg.lineWidth = 3;
	g_drawsize.size = sizeof(DRAWLINE_MSG);
	g_erasepicmsg.type = TYPE_ERASEPIC;
	
	// ��ȭ���� ����
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(g_hReadEvent);
	CloseHandle(g_hWriteEvent);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButtonIsIPv6;
	static HWND hEditIPaddr;
	static HWND hEditPort;
	static HWND hButtonConnect;
	static HWND hButtonClose;
	static HWND hBtnSendFile; // ���� �������� ����
	static HWND hEditMsg;
	static HWND hColorButton;
	static HWND hShapeFill;
	static HWND hShapeCombo;
	static HWND hLineWidthSlider;
	static HWND hEditUserID;
	static HWND hBtnErasePic; // ���� �������� ����
	static HWND hStaticDummy;
	static HWND hButtonNotice; //[���� ���] ��ư //����
	
	int cnt = 0;
	CHOOSECOLOR COL;
	COLORREF crTemp[16];
	OPENFILENAME OFN;
	TCHAR str[100], lpstrFile[100] = _T("");
	TCHAR filter[] = _T("Every File(*.*) \0*.*\0Text File\0*.txt;*.doc\0");

	switch(uMsg){
	case WM_INITDIALOG:
		// ��Ʈ�� �ڵ� ���
		g_hButtonIsIPv6 = GetDlgItem(hDlg, IDC_ISIPV6);
		g_hEditIPaddr = GetDlgItem(hDlg, IDC_IPADDR);
		g_hEditPort = GetDlgItem(hDlg, IDC_PORT);
		g_hButtonConnect = GetDlgItem(hDlg, IDC_CONNECT);
		g_hButtonClose = GetDlgItem(hDlg, IDC_CLOSE);
		hBtnSendFile = GetDlgItem(hDlg, IDC_SENDFILE);
		g_hBtnSendFile = hBtnSendFile; // ���� ������ ����
		g_hButtonSendMsg = GetDlgItem(hDlg, IDC_SENDMSG);
		hEditMsg = GetDlgItem(hDlg, IDC_MSG);
		g_hEditStatus = GetDlgItem(hDlg, IDC_STATUS);
		hShapeCombo = GetDlgItem(hDlg, IDC_COMBO_SHAPE);
		g_hEditUserID = GetDlgItem(hDlg, IDC_USERID_EDIT);
		hColorButton = GetDlgItem(hDlg, IDC_COLORBTN);// rc�߰�
		hLineWidthSlider = GetDlgItem(hDlg, IDC_LINEWIDTH);// rc�߰�
		hShapeFill = GetDlgItem(hDlg, IDC_ISFILL_CHECK);   // rc�߰�
		hBtnErasePic = GetDlgItem(hDlg, IDC_ERASER);
		g_hBtnErasePic = hBtnErasePic; // ���� ������ ����
		hStaticDummy = GetDlgItem(hDlg, IDC_DUMMY);
		g_hButtonNotice = GetDlgItem(hDlg, IDC_NOTICE);
		g_hEditNotice = GetDlgItem(hDlg, IDC_EDITNOTICE); 

		// ��Ʈ�� �ʱ�ȭ
		SendMessage(hEditMsg, EM_SETLIMITTEXT, MSGSIZE, 0);
		SendMessage(g_hEditUserID, EM_SETLIMITTEXT, IDSIZE, 0);
		EnableWindow(g_hBtnSendFile, FALSE);
		EnableWindow(g_hButtonSendMsg, FALSE);
		EnableWindow(g_hButtonNotice, FALSE);
		EnableWindow(g_hButtonClose, FALSE);
		SetDlgItemText(hDlg, IDC_IPADDR, (LPCSTR)SERVERIPV4);
		SetDlgItemText(hDlg, IDC_USERID_EDIT, (LPCSTR)DEFAULTID);
		SetDlgItemInt(hDlg, IDC_PORT, SERVERPORT, FALSE);
		EnableWindow(g_hBtnErasePic, FALSE);
		SendMessage(hLineWidthSlider, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(1, 7));
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"��");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"�ﰢ��");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"�簢��");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"Ÿ����");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"����");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"������");
		SendMessage(hShapeCombo, CB_ADDSTRING, 0, (LPARAM)"���찳");
		SendMessage(hShapeCombo, CB_SETCURSEL, 0, (LPARAM)"��");
	

		// ������ Ŭ���� ���
		WNDCLASS wndclass;
		wndclass.style = CS_HREDRAW|CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = g_hInst;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "MyWndClass";
		if(!RegisterClass(&wndclass)) return 1;

		// �ڽ� ������ ����
		RECT rect; GetWindowRect(hStaticDummy, &rect);
		POINT pt; pt.x = rect.left; pt.y = rect.top;
		ScreenToClient(hDlg, &pt);

		g_hDrawWnd = CreateWindow("MyWndClass", "�׸� �׸� ������", WS_CHILD,
			430, 83, 425, 415, hDlg, (HMENU)NULL, g_hInst, NULL);
		if(g_hDrawWnd == NULL) return 1;
		ShowWindow(g_hDrawWnd, SW_SHOW);
		UpdateWindow(g_hDrawWnd);

		return TRUE;

	case WM_HSCROLL:
		g_drawmsg.lineWidth = SendDlgItemMessage(hDlg, IDC_LINEWIDTH, TBM_GETPOS, 0, 0);
		//g_drawmsg.lineWidth = pos;
		return 0;

	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ISIPV6:
			g_isIPv6 = SendMessage(g_hButtonIsIPv6, BM_GETCHECK, 0, 0);
			if(g_isIPv6 == false)
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV4);
			else
				SetDlgItemText(hDlg, IDC_IPADDR, SERVERIPV6);
			return TRUE;

		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_IPADDR, g_ipaddr, sizeof(g_ipaddr));
			g_port = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);
			g_isIPv6 = SendMessage(g_hButtonIsIPv6, BM_GETCHECK, 0, 0);
			GetDlgItemText(hDlg, IDC_USERID_EDIT, (LPSTR)g_chatmsg.nickname, IDSIZE);
			GetDlgItemText(hDlg, IDC_USERID_EDIT, (LPSTR)g_noticemsg.nickname, IDSIZE);

			// ���� ��� ������ ����
			g_hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
			if(g_hClientThread == NULL){
				MessageBox(hDlg, "Ŭ���̾�Ʈ�� ������ �� �����ϴ�."
					"\r\n���α׷��� �����մϴ�.", "����!", MB_ICONERROR);
				EndDialog(hDlg, 0);
			}
			else {
				EnableWindow(g_hButtonConnect, FALSE);
				while (g_bStart == FALSE); // ���� ���� ���� ��ٸ�
				EnableWindow(g_hButtonIsIPv6, FALSE);
				EnableWindow(g_hEditIPaddr, FALSE);
				EnableWindow(g_hEditPort, FALSE);
				EnableWindow(g_hBtnSendFile, TRUE);
				EnableWindow(g_hButtonSendMsg, TRUE);
				EnableWindow(g_hEditUserID, FALSE);
				EnableWindow(g_hButtonClose, TRUE);
				EnableWindow(g_hBtnErasePic, TRUE);
				EnableWindow(g_hButtonNotice, TRUE);
				SetFocus(hEditMsg);
			}
			return TRUE;

		case IDC_NOTICE: //���� ��ư �̺�Ʈ
			chat = 0;
			WaitForSingleObject(g_hReadEvent, INFINITE);
			GetDlgItemText(hDlg, IDC_MSG, g_noticemsg.buf, MSGSIZE); 
			GetDlgItemText(hDlg, IDC_USERID_EDIT, (LPSTR)g_noticemsg.nickname, IDSIZE);
			SetEvent(g_hWriteEvent);
			SetWindowText(hEditMsg, 0);
			return TRUE;

		case IDC_SENDMSG: //�޼��� ���� ��ư �̺�Ʈ
			chat = 1;
			// �б� �ϷḦ ��ٸ�
			WaitForSingleObject(g_hReadEvent, INFINITE);
			GetDlgItemText(hDlg, IDC_MSG, g_chatmsg.buf, MSGSIZE);
			// ���� �ϷḦ �˸�
			SetEvent(g_hWriteEvent);
			// �Էµ� �ؽ�Ʈ ��ü�� ���� ǥ��
			SetWindowText(hEditMsg, 0); 
			return TRUE;

		case IDC_ERASERPIC:
			send(g_sock, (char*)&g_erasepicmsg, sizeof(COMM_MSG), 0);
			return TRUE;

		case IDCANCEL:
			if (MessageBox(hDlg, "������ �����Ͻðڽ��ϱ�?",
				"����", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				closesocket(g_sock);
				EndDialog(hDlg, IDCANCEL);
			}
			return TRUE;

		case IDC_CLOSE :
		{
			closesocket(g_sock);
			g_bStart = FALSE;
			EnableWindow(g_hButtonClose, FALSE);
			EnableWindow(g_hButtonConnect, TRUE);
			EnableWindow(g_hButtonIsIPv6, TRUE);
			EnableWindow(g_hBtnSendFile, FALSE);
			EnableWindow(g_hEditIPaddr, TRUE);
			EnableWindow(g_hEditPort, TRUE);
			EnableWindow(g_hButtonSendMsg, FALSE);
			EnableWindow(g_hEditUserID, TRUE);
			EnableWindow(g_hBtnErasePic, FALSE);
			EnableWindow(g_hButtonNotice, FALSE);

			return TRUE;
		}

		case IDC_COMBO_SHAPE:		// ��� combobox ������ �� 
			switch (ComboBox_GetCurSel(hShapeCombo))
			{
			case 0:	g_drawmsg.type = DRAWLINE; break;
			case 1:	g_drawmsg.type = DRAWTRI; break;
			case 2:	g_drawmsg.type = DRAWRECT; break;
			case 3:	g_drawmsg.type = DRAWELLIPSE; break;
			case 4: g_drawmsg.type = DRAWCIRCLE; break;
			case 5:	g_drawmsg.type = DRAWSTRICTLINE; break;
			case 6:	g_drawmsg.type = DRAWERASER; break;
			default:
				g_drawmsg.type = DRAWLINE;
				break;
			}

		case IDC_ISFILL_CHECK:
			g_drawmsg.isFill = SendMessage(hShapeFill, BM_GETCHECK, 0, 0);
			return TRUE;

		case IDC_COLORBTN:
			memset(&COL, 0, sizeof(CHOOSECOLOR));
			COL.lStructSize = sizeof(CHOOSECOLOR);
			COL.hwndOwner = hDlg;
			COL.lpCustColors = crTemp;
			COL.Flags = 0;

			if (ChooseColor((LPCHOOSECOLORA)&COL) != 0) {
				Color = COL.rgbResult;
				g_drawmsg.color = Color;
			}
			return TRUE;

		}
		return FALSE;
	}

	return FALSE;
}

// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;
	if (g_isIPv6 == false) { // TCPv4
		// socket()
		g_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (g_sock == INVALID_SOCKET) err_quit("socket()");

		// connect()
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr(g_ipaddr);
		serveraddr.sin_port = htons(g_port);
		retval = connect(g_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		send(g_sock, g_chatmsg.nickname, IDSIZE, 0);
		if (retval == SOCKET_ERROR) err_quit("connect()");
	}
	else {
		// socket()
		g_sock = socket(AF_INET6, SOCK_STREAM, 0);
		if (g_sock == INVALID_SOCKET) err_quit("socket()");

		// connect()
		SOCKADDR_IN6 serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin6_family = AF_INET6;
		int addrlen = sizeof(serveraddr);
		WSAStringToAddress(g_ipaddr, AF_INET6, NULL,
			(SOCKADDR*)&serveraddr, &addrlen);
		serveraddr.sin6_port = htons(g_port);
		retval = connect(g_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		send(g_sock, g_chatmsg.nickname, IDSIZE, 0);
		if (retval == SOCKET_ERROR) err_quit("connect()");
	}
	

	MessageBox(NULL, "������ �����߽��ϴ�.", "����!", MB_ICONINFORMATION);

	// �б� & ���� ������ ����
	HANDLE        hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);
	if(hThread[0] == NULL || hThread[1] == NULL){
		MessageBox(NULL, "�����带 ������ �� �����ϴ�."
			"\r\n���α׷��� �����մϴ�.",
			"����!", MB_ICONERROR);
		exit(1);
	}

	g_bStart = TRUE;
	
	// ������ ���� ���
	retval = WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	retval -= WAIT_OBJECT_0;
	if(retval == 0)
		TerminateThread(hThread[1], 1);
	else
		TerminateThread(hThread[0], 1);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);


	MessageBox(NULL, "������ ������ �������ϴ�", "�˸�", MB_ICONINFORMATION);
	EnableWindow(g_hButtonSendMsg, FALSE);
	EnableWindow(g_hBtnErasePic, FALSE);
	EnableWindow(g_hButtonClose, FALSE);
	EnableWindow(g_hButtonConnect, TRUE);
	EnableWindow(g_hButtonIsIPv6, TRUE);
	EnableWindow(g_hEditIPaddr, TRUE);
	EnableWindow(g_hEditPort, TRUE);
	EnableWindow(g_hEditUserID, TRUE);
	EnableWindow(g_hBtnErasePic, FALSE);
	EnableWindow(g_hButtonNotice, FALSE); //����
	EnableWindow(g_hBtnSendFile, FALSE);

	closesocket(g_sock);
	return 0;
}

// ������ �ޱ�
DWORD WINAPI ReadThread(LPVOID arg)
{
	int retval;
	char buf[512];
	COMM_MSG comm_msg;
	CHAT_MSG chat_msg;
	NOTICE_MSG notice_msg;
	DRAWLINE_MSG draw_msg;
	ERASEPIC_MSG erasepic_msg;
	struct sockaddr_in clientaddr;

	// ������ ������ ��� //����
	while (1) { 
		retval = recvn(g_sock, (char*)&comm_msg, sizeof(COMM_MSG), 0);
		if (retval == 0 || retval == SOCKET_ERROR) {
			break;
		}
		if (comm_msg.type == NOTICE) {
			retval = recvn(g_sock, (char*)&notice_msg, comm_msg.size, 0); //����
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			char* pch;
			char* pch2;
			pch = strstr(notice_msg.buf, "�ٺ�");
			pch2 = strstr(notice_msg.buf, "��û");

			if (pch || pch2) {
				DisplayText("[%s] %s\r\n", notice_msg.nickname, "������ ���� �Ͻø� �ȵ˴ϴ�.");
			}
			else {
				SetWindowText(g_hEditNotice, 0);//����
				DisplayText("[����][%s] %s\r\n", notice_msg.nickname, notice_msg.buf);
				DisplayNOTICE("[%s] %s\r\n", notice_msg.nickname, notice_msg.buf);
			}
			//chat = 1;
		}
		else if (comm_msg.type == CHATTING) {
			retval = recvn(g_sock, (char*)&chat_msg, comm_msg.size, 0);
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			char* pch1;
			char* pch2;
			pch1 = strstr(chat_msg.buf, "�ٺ�");
			pch2 = strstr(chat_msg.buf, "��û");
				
			char* change1;
			char* change2;
			char* change3;

			change1 = replaceAll(chat_msg.buf, "�ٺ�", "**");
			change2 = replaceAll(chat_msg.buf, "��û", "**");
			change3 = replaceAll(change1, "��û", "**");

			if (pch1 && !pch2) {
				DisplayText("[%s] %s\r\n", chat_msg.nickname, change1);
			}
			else if (pch2 && !pch1) {
				DisplayText("[%s] %s\r\n", chat_msg.nickname, change2);
			}
			else if (pch1 && pch2) {
				DisplayText("[%s] %s\r\n", chat_msg.nickname, change3);
			}
			else {
				DisplayText("[%s] %s\r\n", chat_msg.nickname, chat_msg.buf);
			}
		}

		else if (comm_msg.type == TYPE_ERASEPIC) {

			retval = recvn(g_sock, (char*)&erasepic_msg, comm_msg.size, 0);
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			SendMessage(g_hDrawWnd, WM_ERASEPIC, 0, 0);
		}

		else if (comm_msg.type == DRAWLINE) {
			retval = recvn(g_sock, (char*)&draw_msg, comm_msg.size, 0);
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			g_drawcolor = draw_msg.color;


			SendMessage(g_hDrawWnd, WM_DRAWIT,
				(WPARAM)&draw_msg,
				MAKELPARAM(draw_msg.x1, draw_msg.y1));
		}

		else if (comm_msg.type == DRAWERASER) {
			retval = recvn(g_sock, (char*)&draw_msg, comm_msg.size, 0);
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			g_drawcolor = RGB(255, 255, 255);

			SendMessage(g_hDrawWnd, WM_DRAWIT,
				(WPARAM)&draw_msg, MAKELPARAM(draw_msg.x1, draw_msg.y1));
		}


		else {
			retval = recvn(g_sock, (char*)&draw_msg, comm_msg.size, 0);
			if (retval == 0 || retval == SOCKET_ERROR) {
				break;
			}
			g_drawcolor = draw_msg.color;

			SendMessage(g_hDrawWnd, WM_DRAWIT,
				(WPARAM)&draw_msg, MAKELPARAM(draw_msg.x1, draw_msg.y1));
		}
	}
	return 0;
}

// ������ ������
DWORD WINAPI WriteThread(LPVOID arg)
{
	int retval;
	COMM_MSG msg_size;

	// ������ ������ ���
	while(1){ // ----------����------------
		WaitForSingleObject(g_hWriteEvent, INFINITE);
		if (chat == 0) {
			// ���ڿ� ���̰� 0�̸� ������ ����
			if (strlen(g_noticemsg.buf) == 0) {
				// '�޽��� ����' ��ư Ȱ��ȭ
				EnableWindow(g_hButtonNotice, TRUE);
				// �б� �Ϸ� �˸���
				SetEvent(g_hReadEvent);
				continue;
			}
			msg_size.size = strlen(g_noticemsg.buf) + 1 + IDSIZE + sizeof(int);
			msg_size.type = g_noticemsg.type;

			retval = send(g_sock, (char*)&msg_size, sizeof(COMM_MSG), 0);
			if (retval == SOCKET_ERROR) {
				break;
			}
			// ������ ������
			retval = send(g_sock, (char*)&g_noticemsg, msg_size.size, 0);
			if (retval == SOCKET_ERROR) {
				break;
			}			
			// '�޽��� ����' ��ư Ȱ��ȭ
			EnableWindow(g_hButtonNotice, TRUE);
			// �б� �Ϸ� �˸���
			SetEvent(g_hReadEvent);
		}

		else { // -------------ä��-------------
			// ���ڿ� ���̰� 0�̸� ������ ����
			if (strlen(g_chatmsg.buf) == 0) {
				// '�޽��� ����' ��ư Ȱ��ȭ
				EnableWindow(g_hButtonSendMsg, TRUE);
				// �б� �Ϸ� �˸���
				SetEvent(g_hReadEvent);
				continue;
			}
			msg_size.size = strlen(g_chatmsg.buf) + 1 + IDSIZE + sizeof(int);
			msg_size.type = g_chatmsg.type;

			retval = send(g_sock, (char*)&msg_size, sizeof(COMM_MSG), 0);
			if (retval == SOCKET_ERROR) {
				break;
			}
			// ������ ������
			retval = send(g_sock, (char*)&g_chatmsg, msg_size.size, 0);
			if (retval == SOCKET_ERROR) {
				break;
			}
			// '�޽��� ����' ��ư Ȱ��ȭ
			EnableWindow(g_hButtonSendMsg, TRUE);
			// �б� �Ϸ� �˸���
			SetEvent(g_hReadEvent);
		}
	}
	return 0;
}

// �ڽ� ������ ���ν���
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	static int cx, cy;
	PAINTSTRUCT ps;
	RECT rect;
	HPEN hPen, hOldPen;
	static HBITMAP hBitmap;
	static HDC hDCMem;
	static int x0, y0;
	static int x1, y1;
	static BOOL bDrawing = FALSE;
	HBRUSH MyBrush, OldBrush;
	LOGBRUSH LogBrush;
	static POINT ptLast = { 0, 0 };  // ���������� ������ ��ġ ����
	const int SEND_THRESHOLD = 30;  // �̵� �Ÿ��� �Ӱ谪


	switch(uMsg){
	case WM_CREATE:
		hDC = GetDC(hWnd);

		// ȭ���� ������ ��Ʈ�� ����
		cx = GetDeviceCaps(hDC, HORZRES);
		cy = GetDeviceCaps(hDC, VERTRES);
		//cx = LOWORD(lParam);
		//cy = HIWORD(lParam);
		hBitmap = CreateCompatibleBitmap(hDC, cx, cy);

		// �޸� DC ����
		hDCMem = CreateCompatibleDC(hDC);

		// ��Ʈ�� ���� �� �޸� DC ȭ���� ������� ĥ��
		SelectObject(hDCMem, hBitmap);
		SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
		SelectObject(hDCMem, GetStockObject(WHITE_PEN));
		Rectangle(hDCMem, 0, 0, cx, cy);

		ReleaseDC(hWnd, hDC);
		return 0;
	case WM_LBUTTONDOWN:
		x0 = LOWORD(lParam);
		y0 = HIWORD(lParam);
		bDrawing = TRUE;
		return 0;
	case WM_MOUSEMOVE:
		if (bDrawing && g_bStart) {
			int x1 = LOWORD(lParam);
			int y1 = HIWORD(lParam);

			int dx = x1 - ptLast.x;
			int dy = y1 - ptLast.y;
			if (sqrt(dx * dx + dy * dy) > SEND_THRESHOLD) {
				// �� �׸��� �޽��� ������
				g_drawmsg.x0 = x0;
				g_drawmsg.y0 = y0;
				g_drawmsg.x1 = x1;
				g_drawmsg.y1 = y1;

				if (g_drawmsg.type == DRAWLINE) {
					g_drawsize.type = DRAWLINE;
					send(g_sock, (char*)&g_drawsize, sizeof(COMM_MSG), 0);
					send(g_sock, (char*)&g_drawmsg, g_drawsize.size, 0);
					x0 = x1;
					y0 = y1;
				}
				else if (g_drawmsg.type == DRAWERASER) {
					g_drawsize.type = DRAWERASER;
					send(g_sock, (char*)&g_drawsize, sizeof(COMM_MSG), 0);
					send(g_sock, (char*)&g_drawmsg, g_drawsize.size, 0);
					x0 = x1;
					y0 = y1;
				}

				ptLast.x = x1;
				ptLast.y = y1;
			}
		}
		return 0;
	case WM_LBUTTONUP:
		if (g_drawmsg.type != DRAWLINE || g_drawmsg.type!= DRAWERASER) {
			g_drawsize.type = g_drawmsg.type;
			send(g_sock, (char*)&g_drawsize, sizeof(COMM_MSG), 0);
			send(g_sock, (char*)&g_drawmsg, g_drawsize.size, 0);
		} 
		bDrawing = FALSE;
		return 0;

	case WM_DRAWIT:
		hDC = GetDC(hWnd);
		DRAWLINE_MSG* newLine = (DRAWLINE_MSG*)wParam;
		hPen = CreatePen(PS_SOLID, newLine->lineWidth, g_drawcolor); // get lineWidth parameter
		//hPen = CreatePen(PS_SOLID, 3, g_drawcolor);

		// ȭ�鿡 �׸���
		hOldPen = (HPEN)SelectObject(hDC, hPen);
		switch (newLine->type)
		{
		case DRAWLINE:
			//MoveToEx(hDC, newLine->x0, newLine->y0, NULL);
			//LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
			//break;
		case DRAWERASER: {
			MoveToEx(hDC, newLine->x0, newLine->y0, NULL);
			LineTo(hDC, LOWORD(lParam), HIWORD(lParam));

			MoveToEx(hDCMem, newLine->x0, newLine->y0, NULL);
			LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
			break;
		
		}
			//MoveToEx(hDC, newLine->x0, newLine->y0, NULL);
			//LineTo(hDC, LOWORD(lParam), HIWORD(lParam));
			//break;
		case DRAWTRI:
		{
			POINT trianglePoints[4] = { 
				{ LOWORD(lParam), HIWORD(lParam) }, { newLine->x0, HIWORD(lParam) }, 
				{ (LOWORD(lParam) - newLine->x0) / 2 + newLine->x0, newLine->y0 }, 
				{ LOWORD(lParam), HIWORD(lParam) } };
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDC, brush);
			}
			else
			{
				SelectObject(hDC, GetStockObject(NULL_BRUSH));
			}

			Polygon(hDC, trianglePoints, 4);
			break;
		}
		case DRAWRECT:
		{
			POINT rectanglePoints[5] = { { LOWORD(lParam), HIWORD(lParam) },
										 { newLine->x0, HIWORD(lParam) },
										 { newLine->x0, newLine->y0 },
										 { LOWORD(lParam), newLine->y0 },
										 { LOWORD(lParam), HIWORD(lParam)} };
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDC, brush);
			}
			else
			{
				SelectObject(hDC, GetStockObject(NULL_BRUSH));
			}
			Polygon(hDC, rectanglePoints, 5);
			break;
		}
		case DRAWELLIPSE://Ÿ���϶�
		{
			POINT ellipse[360];
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDC, brush);
			}
			else
			{
				SelectObject(hDC, GetStockObject(NULL_BRUSH));
			}

			//Ellipse(hDC, newLine->x0, newLine->y0, LOWORD(lParam), HIWORD(lParam));
			double radian = 0.0;
			MoveToEx(hDC, (newLine->x0 - LOWORD(lParam)) / 2 + LOWORD(lParam) + cos(radian) * (newLine->x0 - LOWORD(lParam)) / 2,
				(newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) + sin(radian) * (newLine->x0 - LOWORD(lParam)) / 2, NULL);

			for (int degree = 1; degree <= 360; degree++) {
				radian = degree * PI / 180;
				ellipse[degree - 1] = { (int)((newLine->x0 - LOWORD(lParam)) / 2 
					+ LOWORD(lParam) + cos(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)) ,
				 (int)((newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) 
					 + sin(radian) * ((newLine->y0 - HIWORD(lParam)) / 2)) };
			}
			Polygon(hDC, ellipse, 360);
			break;
		}
		case DRAWCIRCLE://���϶�
		{
			POINT circle[360];
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDC, brush);
			}
			else
			{
				SelectObject(hDC, GetStockObject(NULL_BRUSH));
			}

			//Ellipse(hDC, newLine->x0, newLine->y0, LOWORD(lParam), HIWORD(lParam));
			double radian = 0.0;
			MoveToEx(hDC, (newLine->x0 - LOWORD(lParam)) / 2 + LOWORD(lParam) + cos(radian) * (newLine->x0 - LOWORD(lParam)) / 2,
				(newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) + sin(radian) * (newLine->x0 - LOWORD(lParam)) / 2, NULL);

			for (int degree = 1; degree <= 360; degree++) {
				radian = degree * PI / 180;
				circle[degree - 1] = { (int)((newLine->x0 - LOWORD(lParam)) / 2 
					+ LOWORD(lParam) + cos(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)),
				 (int)((newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) 
					+ sin(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)) };
			}
			Polygon(hDC, circle, 360);
			break;
		}
		case DRAWSTRICTLINE:
		{
			MoveToEx(hDC, newLine->x0, newLine->y0, NULL);
			LineTo(hDC, newLine->x1, newLine->y1);
			break;
		}
		default:
			break;
		}

		// �޸� ��Ʈ�ʿ� �׸���
		hOldPen = (HPEN)SelectObject(hDCMem, hPen);
		switch (newLine->type)
		{
		case DRAWLINE:
			MoveToEx(hDCMem, newLine->x0, newLine->y0, NULL);
			LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
			break;
		case DRAWERASER:
			MoveToEx(hDCMem, newLine->x0, newLine->y0, NULL);
			LineTo(hDCMem, LOWORD(lParam), HIWORD(lParam));
			break;
		case DRAWTRI:
		{
			POINT trianglePoints[4] = { { LOWORD(lParam), HIWORD(lParam) }, { newLine->x0, HIWORD(lParam) }, { (LOWORD(lParam) - newLine->x0) / 2 + newLine->x0, newLine->y0 }, { LOWORD(lParam), HIWORD(lParam) } };

			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDCMem, brush);
			}
			else
			{
				SelectObject(hDCMem, GetStockObject(NULL_BRUSH));
			}

			Polygon(hDCMem, trianglePoints, 4);
			SelectObject(hDCMem, hOldPen);
			break;
		}
		case DRAWRECT:
		{
			POINT rectanglePoints[5] = { { LOWORD(lParam), HIWORD(lParam) },
										 { newLine->x0, HIWORD(lParam) },
										 { newLine->x0, newLine->y0 },
										 { LOWORD(lParam), newLine->y0 },
										 { LOWORD(lParam), HIWORD(lParam)} };
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDC, brush);
			}
			else
			{
				SelectObject(hDCMem, GetStockObject(NULL_BRUSH));
			}
			Polygon(hDCMem, rectanglePoints, 5);
			SelectObject(hDCMem, hOldPen);
			break;
		}
		case DRAWELLIPSE:
		{
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDCMem, brush);
			}
			else
				SelectObject(hDCMem, GetStockObject(NULL_BRUSH));
			
			POINT ellipse[360];
			double radian = 0.0;
			MoveToEx(hDCMem,(newLine->x0 - LOWORD(lParam))/2+LOWORD(lParam) + 
				cos(radian) * (newLine->x0 - LOWORD(lParam)) / 2,
				(newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) + 
				sin(radian) * (newLine->x0 - LOWORD(lParam)) / 2,NULL);

			for (int degree = 1; degree <= 360; degree++) {
				radian = degree * PI / 180;
				for (int degree = 1; degree <= 360; degree++) {
					radian = degree * PI / 180;
					ellipse[degree - 1] = { (int)((newLine->x0 - LOWORD(lParam)) / 2 
						+ LOWORD(lParam) + cos(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)) ,
					 (int)((newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) 
						 + sin(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)) };
				}
			}
			Polygon(hDCMem, ellipse, 360);
			SelectObject(hDCMem, hOldPen);
			break;
		}
		case DRAWCIRCLE:
		{
			if (newLine->isFill)
			{
				LogBrush.lbColor = g_drawcolor;
				LogBrush.lbHatch = HS_VERTICAL;
				LogBrush.lbStyle = BS_SOLID;
				HBRUSH brush = CreateBrushIndirect(&LogBrush);
				SelectObject(hDCMem, brush);
			}
			else
			{
				SelectObject(hDCMem, GetStockObject(NULL_BRUSH));
			}

			POINT circle[360];
			double radian = 0.0;
			MoveToEx(hDCMem, (newLine->x0 - LOWORD(lParam)) / 2 + LOWORD(lParam) 
				+ cos(radian) * (newLine->x0 - LOWORD(lParam)) / 2,
				(newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) 
				+ sin(radian) * (newLine->x0 - LOWORD(lParam)) / 2, NULL);

			for (int degree = 1; degree <= 360; degree++) {
				radian = degree * PI / 180;
				for (int degree = 1; degree <= 360; degree++) {
					radian = degree * PI / 180;
					circle[degree - 1] = { (int)((newLine->x0 - LOWORD(lParam)) / 2 + LOWORD(lParam) 
						+ cos(radian) * ((newLine->x0 - LOWORD(lParam)) / 2)) ,
					 (int)((newLine->y0 - HIWORD(lParam)) / 2 + HIWORD(lParam) 
						 + sin(radian) * ((newLine->y0 - LOWORD(lParam)) / 2)) };
				}
				Polygon(hDCMem, circle, 360);
				SelectObject(hDCMem, hOldPen);
				break;
			}
		}
		case DRAWSTRICTLINE:
		{
			MoveToEx(hDCMem, newLine->x0, newLine->y0, NULL);
			LineTo(hDCMem, newLine->x1, newLine->y1);
			break;
		}
		default:
			break;
			SelectObject(hDC, hOldPen);

			DeleteObject(hPen);
			ReleaseDC(hWnd, hDC);
			return 0;
		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);

			// �޸� ��Ʈ�ʿ� ����� �׸��� ȭ�鿡 ����
			GetClientRect(hWnd, &rect);
			BitBlt(hDC, 0, 0, rect.right - rect.left,
				rect.bottom - rect.top, hDCMem, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);
			return 0;
		case WM_ERASEPIC:
			SelectObject(hDCMem, GetStockObject(WHITE_BRUSH));
			SelectObject(hDCMem, GetStockObject(WHITE_PEN));
			Rectangle(hDCMem, 0, 0, cx, cy);
			//WM_PAINT �޽��� ��������
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;
		case WM_DESTROY:
			DeleteObject(hBitmap);
			DeleteDC(hDCMem);
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ���� ����Ʈ ��Ʈ�ѿ� ���ڿ� ��� //��
void DisplayNOTICE(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditNotice);
	SendMessage(g_hEditNotice, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditNotice, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// ����Ʈ ��Ʈ�ѿ� ���ڿ� ���
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditStatus);
	SendMessage(g_hEditStatus, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditStatus, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
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

//���� ġȯ
char* replaceAll(char* s, const char* olds, const char* news) {
	char* result, * sr;
	size_t i, count = 0;
	size_t oldlen = strlen(olds); if (oldlen < 1) return s;
	size_t newlen = strlen(news);


	if (newlen != oldlen) {
		for (i = 0; s[i] != '\0';) {
			if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
			else i++;
		}
	}
	else i = strlen(s);


	result = (char*)malloc(i + 1 + count * (newlen - oldlen));
	if (result == NULL) return NULL;


	sr = result;
	while (*s) {
		if (memcmp(s, olds, oldlen) == 0) {
			memcpy(sr, news, newlen);
			sr += newlen;
			s += oldlen;
		}
		else *sr++ = *s++;
	}
	*sr = '\0';

	return result;
}