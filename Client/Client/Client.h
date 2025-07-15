#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "ClientPacket.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

class Client
{
public:
    Client();
    ~Client();
    void Ready();
    void Run(char* name);

private:
    int retval;
    WSADATA wsa;
    SOCKET sock;
    // ������ ��ſ� ����� ����
    char buf[BUFSIZE + 1];
    int len;

    char* MyIP();
    static void Err_quit(const char* msg);
    static void Err_display(const char* msg);
    static DWORD WINAPI ThreadRecv(LPVOID arg);
};