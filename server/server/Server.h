#define _CRT_SECURE_NO_WARNINGS         // window에서 sprinf와 localtime 경고 비활성화
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 이전 버전의 Winsock 함수 사용 시 경고를 비활성화합니다.
#pragma comment(lib, "ws2_32")          // Winsock2 라이브러리(ws2_32.lib)를 링크합니다.
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <vector>
#include <mutex>
#include <queue>
#include <algorithm>
#include "ServerPacket.h"

#define SERVERPORT 9000 // 서버가 사용할 포트 번호

using namespace std;

// 각 클라이언트의 상태 정보를 저장하기 위한 구조체
struct SOCKETINFO {
    OVERLAPPED overlapped;      // 비동기 I/O 작업을 위한 OVERLAPPED 구조체. 반드시 첫 번째 멤버여야 함
    SOCKET sock;                // 클라이언트와 연결된 소켓
    char buf[BUFSIZE + 1];      // 데이터 수신/송신 버퍼
    int recvbytes;              // 수신한 데이터 바이트 수 (또는 송신할 총 바이트 수)
    int sendbytes;              // 송신한 데이터 바이트 수
    WSABUF wsabuf;              // 비동기 입출력 함수에서 사용할 버퍼 정보
    bool sending;               // 현재 데이터 송신 작업이 진행 중인지 여부를 나타내는 플래그
    queue<string> sendQueue;    // 송신할 메시지들을 저장하는 큐. 여러 메시지가 동시에 요청될 때 순서를 보장함
    char name[20];              // 클라이언트 별명
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

