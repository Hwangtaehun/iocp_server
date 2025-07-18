#define _CRT_SECURE_NO_WARNINGS         // window���� sprinf�� localtime ��� ��Ȱ��ȭ
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ������ Winsock �Լ� ��� �� ��� ��Ȱ��ȭ�մϴ�.
#pragma comment(lib, "ws2_32")          // Winsock2 ���̺귯��(ws2_32.lib)�� ��ũ�մϴ�.
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <vector>
#include <mutex>
#include <queue>
#include <algorithm>
#include "ServerPacket.h"

#define SERVERPORT 9000 // ������ ����� ��Ʈ ��ȣ

using namespace std;

// �� Ŭ���̾�Ʈ�� ���� ������ �����ϱ� ���� ����ü
struct SOCKETINFO {
    OVERLAPPED overlapped;      // �񵿱� I/O �۾��� ���� OVERLAPPED ����ü. �ݵ�� ù ��° ������� ��
    SOCKET sock;                // Ŭ���̾�Ʈ�� ����� ����
    char buf[BUFSIZE + 1];      // ������ ����/�۽� ����
    int recvbytes;              // ������ ������ ����Ʈ �� (�Ǵ� �۽��� �� ����Ʈ ��)
    int sendbytes;              // �۽��� ������ ����Ʈ ��
    WSABUF wsabuf;              // �񵿱� ����� �Լ����� ����� ���� ����
    bool sending;               // ���� ������ �۽� �۾��� ���� ������ ���θ� ��Ÿ���� �÷���
    queue<string> sendQueue;    // �۽��� �޽������� �����ϴ� ť. ���� �޽����� ���ÿ� ��û�� �� ������ ������
    char name[20];              // Ŭ���̾�Ʈ ����
};

#pragma once
class Server
{
public:
	Server();
	~Server();
    void Ready();
    void Run();
private:
    HANDLE hcp;
    WSADATA wsa;
    SOCKET listen_sock;

    static DWORD WINAPI WorkerThread(LPVOID arg);
    static void err_quit(char* msg);
    static void err_display(char* msg);
    static bool send(SOCKETINFO* ptr);
    static bool receive(SOCKETINFO* ptr);
    static void broadcast(SOCKETINFO* sender, const char* msg, int len);
    static void remove_client(SOCKETINFO* ptr);
};

