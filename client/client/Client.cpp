
#include "Client.h"

Client::Client()
{
    len = 0;
    retval = 0;
    wsa = {};
    sock = INVALID_SOCKET;
    memset(buf, 0, sizeof(buf));

    // 1. 윈속 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;
}

Client::~Client()
{
    // 7. 소켓 닫기 및 윈속
    closesocket(sock);
    WSACleanup();
}

void Client::Ready()
{
    // 2. 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 3. 서버에 접속 (connect)
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

    if (retval == SOCKET_ERROR) err_quit("connect()");
}

void Client::Run(char* name)
{
    HANDLE hThread; // 수신 스레드의 핸들을 저장할 변수
    ClientPacket pk = ClientPacket(name);

    // 4. [프로토콜] 서버 접속 후, 가장 먼저 서버가 보내는 환영 메시지를 받습니다.
    // 이 작업이 끝나야 사용자가 채팅을 시작할 수 있습니다.
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
        closesocket(sock);
        WSACleanup();
        return;
    }
    buf[retval] = '\0';
    printf("%s\n", buf);


    pk.SetConnect(MyIP());
    pk.GetBuf(buf);

    retval = send(sock, buf, pk.GetSize(), 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
        return;
    }


    buf[retval] = '\0';
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

    if (hThread != NULL) { CloseHandle(hThread); }
}

char* Client::MyIP() {
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
void Client::err_quit(char* msg)
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
void Client::err_display(char* msg)
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
DWORD __stdcall Client::ThreadRecv(LPVOID arg) {
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
        printf("[받은 데이터] %s\n", buf);
    }
    return 0;
}