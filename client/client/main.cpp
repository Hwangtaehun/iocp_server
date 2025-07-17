#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 C++ 컴파일러에서 이전 Winsock 함수 사용 시 발생하는 경고를 비활성화합니다.
#define _CRT_SECURE_NO_WARNINGS         // C 표준 함수(sprintf 등) 사용 시 발생하는 보안 경고를 비활성화합니다.
#pragma comment(lib, "ws2_32")          // Winsock2 라이브러리(ws2_32.lib)를 링크합니다.
#include <winsock2.h>
#include <time.h>
#include "ClientPacket.h"

#define SERVERIP   "127.0.0.1" // 접속할 서버의 IP 주소
#define SERVERPORT 9000        // 접속할 서버의 포트 번호
#define BUFSIZE    512         // 데이터 송수신을 위한 버퍼 크기

char* MyIP();
void err_quit(char* msg);
void err_display(char* msg);
DWORD __stdcall ThreadRecv(LPVOID arg);

// 메인 함수: 프로그램의 시작점
int main(int argc, char* argv[])
{
    char name[20];
    printf("이름을 입력해주세요.");
    scanf("%s", &name);
    getchar();

    int retval;

    // 1. 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 2. 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 3. 서버에 접속 (connect)
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

    if (retval == SOCKET_ERROR) err_quit("connect()");

    //printf("[알림] 서버 접속 성공!\n");

    int len;
    char buf[BUFSIZE + 1]; // 데이터 통신에 사용할 변수
    HANDLE hThread; // 수신 스레드의 핸들을 저장할 변수
    ClientPacket pk = ClientPacket(name);

    // 4. [프로토콜] 서버 접속 후, 가장 먼저 서버가 보내는 환영 메시지를 받습니다.
    // 이 작업이 끝나야 사용자가 채팅을 시작할 수 있습니다.
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    pk.SetConnect(MyIP());
    pk.GetBuf(buf);

    retval = send(sock, buf, pk.GetSize(), 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
        return 1;
    }


    buf[retval] = '\0';
    pk.RecvMsg(buf);
    pk.GetBuf(buf);
    printf("%s\n", buf); // 서버가 보낸 환영 메시지 출력
    printf("M: 위치변경, C: 대화, X: 종료로 원하는 메뉴를 선택해주세요.\n");

    // 5. 수신용 스레드를 생성합니다. 이제부터 데이터 수신은 저 스레드가 전담합니다.
    // 세 번째 인자로 스레드가 실행할 함수(ThreadRecv)를,
    // 네 번째 인자로 그 함수에 전달할 인자(sock)를 넘겨줍니다.
    hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sock, 0, NULL);

    // 6. 메인 스레드는 사용자 입력을 받아 서버로 데이터를 '송신'하는 역할을 합니다.
    while (1) {
        char me;
        if (!pk.Check()) {
            me = 'M';
        }
        else {
            //메뉴 선택
            scanf(" %c", &me);
            getchar();
        }


        if (me == 'X') {
            pk.SetClose(MyIP());
            pk.GetBuf(buf);
        }
        else if (me != 'C' && me != 'M') {
            printf("잘못 입력했습니다.\n");
            continue;
        }
        else {
            // 데이터 입력
            printf("\n[보낼 데이터] ");
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                me = 'X';

            // '\n' 문자 제거
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';//null 문자까지 처리
            if (strlen(buf) == 0)
                me = 'X';

            if (me == 'M') {
                pk.SetMove(MyIP(), buf);
                if (!pk.Check()) {
                    printf("좌표를 잘못 입력했습니다. 다시 입력해주세요.");
                    continue;
                }
                pk.GetBuf(buf);
            }
            else if (me == 'C') {
                pk.SendMsg(buf);
                pk.GetBuf(buf);
            }
            else {
                pk.SetClose(MyIP());
                pk.GetBuf(buf);
            }
        }

        // 데이터 보내기
        retval = send(sock, buf, pk.GetSize(), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        if (me == 'X') {
            break;
        }
    }

    // 7. 소켓 닫기 및 윈속 및 쓰레드 종료
    if (hThread != NULL) CloseHandle(hThread);
    closesocket(sock);
    WSACleanup();
    return 0;
}

char* MyIP() {
    char* ip = {};

    if (SERVERIP != "127.0.0.1") {
        char hostname[256];
        struct hostent* hostinf;

        // 호스트 이름 가져오기
        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            err_quit("host name error");
        }

        // 호스트 정보 가져오기
        if ((hostinf = gethostbyname(hostname)) == NULL) {
            err_quit("host entry error");
        }

        // IP 주소 가져오기 및 출력
        for (int i = 0; hostinf->h_addr_list[i] != 0; ++i) {
            ip = inet_ntoa(*(struct in_addr*)hostinf->h_addr_list[i]);
        }
    }
    else {
        ip = "127.0.0.1";
    }

    return ip;
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
    char buf[BUFSIZE + 1];
    Packet pk = Packet();
    SOCKET sock = (SOCKET)arg; // 메인 스레드에서 전달받은 소켓 핸들

    // 이 루프는 프로그램이 종료될 때까지 계속 실행되며 수신을 대기합니다.
    while (1) {
        // recv(): 서버로부터 데이터를 받을 때까지 이 지점에서 대기(블로킹)합니다.
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            // 수신 중 오류 발생
            err_display("recv()");
            break;
        }
        else if (retval == 0) {
            // 서버가 연결을 정상적으로 종료함
            printf("\n[알림] 서버와의 연결이 끊겼습니다.\n");
            break;
        }

        buf[retval] = '\0';
        pk.RecvMsg(buf);
        pk.GetData(buf);

        // 받은 데이터를 화면에 출력
        buf[retval] = '\0'; // 수신한 데이터 끝에 NULL 문자를 추가하여 문자열로 만듦
        printf("\n[받은 데이터] %s", buf);
    }
    return 0;
}
