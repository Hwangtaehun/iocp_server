#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 C++ 컴파일러에서 이전 Winsock 함수 사용 시 발생하는 경고를 비활성화합니다.
#define _CRT_SECURE_NO_WARNINGS         // C 표준 함수(sprintf 등) 사용 시 발생하는 보안 경고를 비활성화합니다.
#pragma comment(lib, "ws2_32")          // Winsock2 라이브러리(ws2_32.lib)를 링크합니다.
#include <winsock2.h>
#include <time.h>
#include "ClientPacket.h"

#define SERVERIP   "127.0.0.1" // 접속할 서버의 IP 주소
#define SERVERPORT 9000        // 접속할 서버의 포트 번호

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
	char buf[BUFSIZE + 1]; // 데이터 통신에 사용할 변수

	char* MyIP();
	static void err_quit(char* msg);
	static void err_display(char* msg);
	static DWORD __stdcall ThreadRecv(LPVOID arg);
};
