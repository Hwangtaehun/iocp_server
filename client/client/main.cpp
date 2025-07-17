#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 C++ 컴파일러에서 이전 Winsock 함수 사용 시 발생하는 경고를 비활성화합니다.
#define _CRT_SECURE_NO_WARNINGS         // C 표준 함수(sprintf 등) 사용 시 발생하는 보안 경고를 비활성화합니다.
#pragma comment(lib, "ws2_32")          // Winsock2 라이브러리(ws2_32.lib)를 링크합니다.
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SERVERIP   "127.0.0.1" // 접속할 서버의 IP 주소
#define SERVERPORT 9000        // 접속할 서버의 포트 번호
#define BUFSIZE    512         // 데이터 송수신을 위한 버퍼 크기

void err_quit(char* msg);
void err_display(char* msg);
DWORD __stdcall ThreadRecv(LPVOID arg);

// 메인 함수: 프로그램의 시작점
int main(int argc, char* argv[])
{
    int retval;
    char msg[9];
    HANDLE hThread; // 수신 스레드의 핸들을 저장할 변수

    // 1. 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 2. 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sprintf_s(msg, "socket");
    if (sock == INVALID_SOCKET) err_quit(msg);

    // 3. 서버에 접속 (connect)
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    sprintf_s(msg, "connect");
    if (retval == SOCKET_ERROR) err_quit(msg);

    printf("[알림] 서버 접속 성공!\n");

    // 데이터 통신에 사용할 변수
    char buf[BUFSIZE + 1];
    int len;

    // 4. [프로토콜] 서버 접속 후, 가장 먼저 서버가 보내는 환영 메시지를 받습니다.
    // 이 작업이 끝나야 사용자가 채팅을 시작할 수 있습니다.
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        sprintf_s(msg, "recv");
        err_display(msg);
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    buf[retval] = '\0';
    printf("%s\n", buf); // 서버가 보낸 환영 메시지 출력

    // 5. 수신용 스레드를 생성합니다. 이제부터 데이터 수신은 저 스레드가 전담합니다.
    // 세 번째 인자로 스레드가 실행할 함수(ThreadRecv)를,
    // 네 번째 인자로 그 함수에 전달할 인자(sock)를 넘겨줍니다.
    hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sock, 0, NULL);

    // 6. 메인 스레드는 사용자 입력을 받아 서버로 데이터를 '송신'하는 역할을 합니다.
    while (1) {
        // 데이터 입력
        printf("\n[보낼 데이터] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        // fgets로 입력받은 문자열 끝에는 '\n'이 포함되므로 이를 제거합니다.
        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0) // 아무것도 입력하지 않았다면 루프 종료
            break;

        // 데이터 보내기
        retval = send(sock, buf, strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            sprintf_s(msg, "send");
            err_display(msg);
            break;
        }
    }

    // 7. 소켓 닫기 및 윈속 종료
    closesocket(sock);
    WSACleanup();
    return 0;
}

// 소켓 함수에서 오류 발생 시, 오류 메시지를 메시지 박스로 출력하고 프로그램을 종료하는 함수
void err_quit(char* msg)
{
    LPVOID lpMsgBuf;
    // WSAGetLastError()를 통해 얻은 시스템 오류 코드를 사람이 읽을 수 있는 문자열로 변환
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf); // FormatMessage가 할당한 메모리 해제
    exit(1);
}

// 소켓 함수에서 오류 발생 시, 콘솔에 오류 메시지를 출력하는 함수
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

// [핵심] 서버로부터 데이터를 수신하는 역할을 전담하는 스레드 함수
DWORD __stdcall ThreadRecv(LPVOID arg) {
    int retval;
    char msg[9]; // 오류 메시지 제목을 저장할 짧은 버퍼
    char buf[BUFSIZE + 1];
    SOCKET sock = (SOCKET)arg; // 메인 스레드에서 전달받은 소켓 핸들

    // 이 루프는 프로그램이 종료될 때까지 계속 실행되며 수신을 대기합니다.
    while (1) {
        // recv(): 서버로부터 데이터를 받을 때까지 이 지점에서 대기(블로킹)합니다.
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            // 수신 중 오류 발생
            sprintf_s(msg, "recv");
            err_display(msg);
            break;
        }
        else if (retval == 0) {
            // 서버가 연결을 정상적으로 종료함
            printf("\n[알림] 서버와의 연결이 끊겼습니다.\n");
            break;
        }

        // 받은 데이터를 화면에 출력
        buf[retval] = '\0'; // 수신한 데이터 끝에 NULL 문자를 추가하여 문자열로 만듦
        printf("\n[받은 데이터] %s", buf);
        printf("\n[보낼 데이터] "); // 사용자 입력을 위해 프롬프트 재출력
    }
    return 0;
}
