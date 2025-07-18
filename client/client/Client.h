#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� C++ �����Ϸ����� ���� Winsock �Լ� ��� �� �߻��ϴ� ��� ��Ȱ��ȭ�մϴ�.
#define _CRT_SECURE_NO_WARNINGS         // C ǥ�� �Լ�(sprintf ��) ��� �� �߻��ϴ� ���� ��� ��Ȱ��ȭ�մϴ�.
#pragma comment(lib, "ws2_32")          // Winsock2 ���̺귯��(ws2_32.lib)�� ��ũ�մϴ�.
#include <winsock2.h>
#include <time.h>
#include "ClientPacket.h"

#define SERVERIP   "127.0.0.1" // ������ ������ IP �ּ�
#define SERVERPORT 9000        // ������ ������ ��Ʈ ��ȣ

class Client
{
public:
	Client();
	~Client();
	void Ready();
	void Run(char* name);
private:
	int len;
	int retval;
	SOCKET sock;
	WSADATA wsa;
	char buf[BUFSIZE + 1]; // ������ ��ſ� ����� ����

	char* MyIP();
	static void err_quit(char* msg);
	static void err_display(char* msg);
	static DWORD __stdcall ThreadRecv(LPVOID arg);
};
