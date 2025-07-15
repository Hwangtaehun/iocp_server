#include "Client.h"

Client::Client()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;
}

Client::~Client()
{
    closesocket(sock);
    // 윈속 종료
    WSACleanup();
}

void Client::Ready()
{
    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) Err_quit("socket()");

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) Err_quit("connect()");
}

void Client::Run(char* name)
{
    ClientPacket pk = ClientPacket(name);
    HANDLE hThread;

    pk.SetConnect(MyIP());
    pk.GetBuf(buf);

    retval = send(sock, buf, pk.GetSize(), 0);
    if (retval == SOCKET_ERROR) {
        Err_display("send()");
        return;
    }

    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        Err_display("recv()");
        return;
    }
    else if (retval == 0)
        return;

    // 받은 데이터 출력
    buf[retval] = '\0';
    pk.RecvMsg(buf);
    pk.GetData(buf);
    printf("%s\n", buf);
    printf("M: 위치변경, C: 대화, X: 종료로 원하는 메뉴를 선택해주세요.\n");

    hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sock, 0, NULL);
    
    // 서버와 데이터 통신
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
        

        if(me == 'X'){
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
            else if(me == 'C'){
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
            Err_display("send()");
            break;
        }

        if (me == 'X') {
            break;
        }
    }

    if (hThread == NULL) { closesocket(sock); }
    else { CloseHandle(hThread); }
}

char* Client::MyIP()
{
    char* ip = {};

    if (SERVERIP != "127.0.0.1") {
        char hostname[256];
        struct hostent* hostinf;

        // 호스트 이름 가져오기
        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            Err_quit("host name error");
        }

        // 호스트 정보 가져오기
        if ((hostinf = gethostbyname(hostname)) == NULL) {
            Err_quit("host entry error");
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

// 소켓 함수 오류 출력 후 종료
void Client::Err_quit(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// 소켓 함수 오류 출력
void Client::Err_display(const char* msg)
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

DWORD __stdcall Client::ThreadRecv(LPVOID arg)
{
    int retval;
    char buf[BUFSIZE + 1];
    Packet pk = Packet();
    SOCKET sock = (SOCKET)arg;
    
    while (1)
    {
        // 데이터 받기
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            Err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        // 받은 데이터 출력
        buf[retval] = '\0';
        pk.RecvMsg(buf);
        pk.GetData(buf);

        printf("[받은 데이터] %s\n", buf);
    }

    return 0;
}
