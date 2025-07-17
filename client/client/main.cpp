#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� C++ �����Ϸ����� ���� Winsock �Լ� ��� �� �߻��ϴ� ��� ��Ȱ��ȭ�մϴ�.
#define _CRT_SECURE_NO_WARNINGS         // C ǥ�� �Լ�(sprintf ��) ��� �� �߻��ϴ� ���� ��� ��Ȱ��ȭ�մϴ�.
#pragma comment(lib, "ws2_32")          // Winsock2 ���̺귯��(ws2_32.lib)�� ��ũ�մϴ�.
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SERVERIP   "127.0.0.1" // ������ ������ IP �ּ�
#define SERVERPORT 9000        // ������ ������ ��Ʈ ��ȣ
#define BUFSIZE    512         // ������ �ۼ����� ���� ���� ũ��

void err_quit(char* msg);
void err_display(char* msg);
DWORD __stdcall ThreadRecv(LPVOID arg);

// ���� �Լ�: ���α׷��� ������
int main(int argc, char* argv[])
{
    int retval;
    char msg[9];
    HANDLE hThread; // ���� �������� �ڵ��� ������ ����

    // 1. ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 2. ���� ����
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sprintf_s(msg, "socket");
    if (sock == INVALID_SOCKET) err_quit(msg);

    // 3. ������ ���� (connect)
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    sprintf_s(msg, "connect");
    if (retval == SOCKET_ERROR) err_quit(msg);

    printf("[�˸�] ���� ���� ����!\n");

    // ������ ��ſ� ����� ����
    char buf[BUFSIZE + 1];
    int len;

    // 4. [��������] ���� ���� ��, ���� ���� ������ ������ ȯ�� �޽����� �޽��ϴ�.
    // �� �۾��� ������ ����ڰ� ä���� ������ �� �ֽ��ϴ�.
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        sprintf_s(msg, "recv");
        err_display(msg);
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    buf[retval] = '\0';
    printf("%s\n", buf); // ������ ���� ȯ�� �޽��� ���

    // 5. ���ſ� �����带 �����մϴ�. �������� ������ ������ �� �����尡 �����մϴ�.
    // �� ��° ���ڷ� �����尡 ������ �Լ�(ThreadRecv)��,
    // �� ��° ���ڷ� �� �Լ��� ������ ����(sock)�� �Ѱ��ݴϴ�.
    hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sock, 0, NULL);

    // 6. ���� ������� ����� �Է��� �޾� ������ �����͸� '�۽�'�ϴ� ������ �մϴ�.
    while (1) {
        // ������ �Է�
        printf("\n[���� ������] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        // fgets�� �Է¹��� ���ڿ� ������ '\n'�� ���ԵǹǷ� �̸� �����մϴ�.
        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0) // �ƹ��͵� �Է����� �ʾҴٸ� ���� ����
            break;

        // ������ ������
        retval = send(sock, buf, strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            sprintf_s(msg, "send");
            err_display(msg);
            break;
        }
    }

    // 7. ���� �ݱ� �� ���� ����
    closesocket(sock);
    WSACleanup();
    return 0;
}

// ���� �Լ����� ���� �߻� ��, ���� �޽����� �޽��� �ڽ��� ����ϰ� ���α׷��� �����ϴ� �Լ�
void err_quit(char* msg)
{
    LPVOID lpMsgBuf;
    // WSAGetLastError()�� ���� ���� �ý��� ���� �ڵ带 ����� ���� �� �ִ� ���ڿ��� ��ȯ
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf); // FormatMessage�� �Ҵ��� �޸� ����
    exit(1);
}

// ���� �Լ����� ���� �߻� ��, �ֿܼ� ���� �޽����� ����ϴ� �Լ�
void err_display(char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// [�ٽ�] �����κ��� �����͸� �����ϴ� ������ �����ϴ� ������ �Լ�
DWORD __stdcall ThreadRecv(LPVOID arg) {
    int retval;
    char msg[9]; // ���� �޽��� ������ ������ ª�� ����
    char buf[BUFSIZE + 1];
    SOCKET sock = (SOCKET)arg; // ���� �����忡�� ���޹��� ���� �ڵ�

    // �� ������ ���α׷��� ����� ������ ��� ����Ǹ� ������ ����մϴ�.
    while (1) {
        // recv(): �����κ��� �����͸� ���� ������ �� �������� ���(���ŷ)�մϴ�.
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            // ���� �� ���� �߻�
            sprintf_s(msg, "recv");
            err_display(msg);
            break;
        }
        else if (retval == 0) {
            // ������ ������ ���������� ������
            printf("\n[�˸�] �������� ������ ������ϴ�.\n");
            break;
        }

        // ���� �����͸� ȭ�鿡 ���
        buf[retval] = '\0'; // ������ ������ ���� NULL ���ڸ� �߰��Ͽ� ���ڿ��� ����
        printf("\n[���� ������] %s", buf);
        printf("\n[���� ������] "); // ����� �Է��� ���� ������Ʈ �����
    }
    return 0;
}
