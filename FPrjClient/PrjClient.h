#pragma once
#pragma comment(lib, "ws2_32")
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
//#include "pch.h"
//#include "tipsware.h"

#define PI			3.141592653589793238
#define SERVERIPV4  "127.0.0.1"
#define SERVERIPV6  "::1"
#define DEFAULTID	"USER"
#define SERVERPORT  9000

#define BUFSIZE     512                    // ���� �޽��� ��ü ũ��
#define IDSIZE		20						//����� ID ����
#define MSGSIZE     (BUFSIZE-sizeof(int)-IDSIZE)  // ä�� �޽��� �ִ� ����

#define CHATTING    1000                   // �޽��� Ÿ��: ä��
#define DRAWLINE    1001                   // �޽��� Ÿ��: �� �׸���
#define DRAWTRI		1002					// �޽��� Ÿ��: �ﰢ�� �׸���
#define DRAWRECT	1003					// �޽��� Ÿ��: �簢�� �׸���
#define DRAWELLIPSE	1004					// �޽��� Ÿ��: Ÿ�� �׸���
#define DRAWSTRICTLINE 1005					// �޽��� Ÿ��: ���� �׸���
#define DRAWCIRCLE  1006					// �޽��� Ÿ��: �� �׸���
#define SERVER_MESSAGE 1111
#define DRAWERASER 1007             // �޽��� Ÿ��: �׸� �����
#define TYPE_ERASEPIC 1008				//�޽��� Ÿ��: �׸� ��ü �����
#define NOTICE 1009                  // �޽��� Ÿ��: ���� //����


#define WM_DRAWIT (WM_USER+1)         // ����� ���� ������ �޽���(1)
#define WM_ERASEPIC (WM_USER+2)         // ����� ���� ������ �޽���(2)

#define SIZE_TOT 256                    // ���� ��Ŷ(��� + ������) ��ü ũ��
#define SIZE_DAT (SIZE_TOT-sizeof(int)) // ����� ������ ������ �κи��� ũ��

int a;
int b;
class RoundShape {
private:
	int rad;
	int x, y;

public:

};
// ���� �޽��� ����
// sizeof(COMM_MSG) == 8
struct COMM_MSG
{
	int  type;
	int  size;
};

// ä�� �޽��� ����
// sizeof(CHAT_MSG) == 256
struct CHAT_MSG
{
	int  type;
	char nickname[IDSIZE];
	char buf[MSGSIZE];
};

struct NOTICE_MSG
{
	int  type;
	char nickname[IDSIZE];
	char buf[MSGSIZE];
};


// �� �׸��� �޽��� ����
// sizeof(DRAWLINE_MSG) == 256
struct DRAWLINE_MSG
{
	int  type;
	int  color;
	int  x0, y0;
	int  x1, y1;
	int lineWidth;
	BOOL isFill;
	//char dummy[BUFSIZE - 8 * sizeof(int)];
};

//�׸� ��ü ����� �޽��� ����
//sizeof(ERASERPIC_MSG)==256
typedef struct ERASEPIC_MSG
{
	int type;
	char dummy[SIZE_DAT];
} ;
//�÷� ���� 

COLORREF Color = RGB(0, 0, 0);

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);
// �ڽ� ������ ���ν���
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char* fmt, ...);
void DisplayNOTICE(char* fmt, ...);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char* buf, int len, int flags);
// ���� ��� �Լ�
void err_quit(char* msg);
void err_display(char* msg);
char* replaceAll(char* s, const char* olds, const char* news);