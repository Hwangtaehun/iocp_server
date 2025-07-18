
#include "Client.h"

Client::Client()
{
    len = 0;
    retval = 0;
    wsa = {};
    sock = INVALID_SOCKET;
    memset(buf, 0, sizeof(buf));

    // 1. ���� �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;
}

Client::~Client()
{
    // 7. ���� �ݱ� �� ����
    closesocket(sock);
    WSACleanup();
}

void Client::Ready()
{
    // 2. ���� ����
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 3. ������ ���� (connect)
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
    HANDLE hThread; // ���� �������� �ڵ��� ������ ����
    ClientPacket pk = ClientPacket(name);

    // 4. [��������] ���� ���� ��, ���� ���� ������ ������ ȯ�� �޽����� �޽��ϴ�.
    // �� �۾��� ������ ����ڰ� ä���� ������ �� �ֽ��ϴ�.
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
    printf("M: ��ġ����, C: ��ȭ, X: ����� ���ϴ� �޴��� �������ּ���.\n");

    // 5. ���ſ� �����带 �����մϴ�. �������� ������ ������ �� �����尡 �����մϴ�.
    // �� ��° ���ڷ� �����尡 ������ �Լ�(ThreadRecv)��,
    // �� ��° ���ڷ� �� �Լ��� ������ ����(sock)�� �Ѱ��ݴϴ�.
    hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sock, 0, NULL);

    // 6. ���� ������� ����� �Է��� �޾� ������ �����͸� '�۽�'�ϴ� ������ �մϴ�.
    while (1) {
        char me;
        if (!pk.Check()) {
            me = 'M';
        }
        else {
            //�޴� ����
            scanf(" %c", &me);
            getchar();
        }


        if (me == 'X') {
            pk.SetClose(MyIP());
            pk.GetBuf(buf);
        }
        else if (me != 'C' && me != 'M') {
            printf("�߸� �Է��߽��ϴ�.\n");
            continue;
        }
        else {
            // ������ �Է�
            printf("\n[���� ������] ");
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                me = 'X';

            // '\n' ���� ����
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';//null ���ڱ��� ó��
            if (strlen(buf) == 0)
                me = 'X';

            if (me == 'M') {
                pk.SetMove(MyIP(), buf);
                if (!pk.Check()) {
                    printf("��ǥ�� �߸� �Է��߽��ϴ�. �ٽ� �Է����ּ���.");
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

        // ������ ������
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

        // ȣ��Ʈ �̸� ��������
        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            err_quit("host name error");
        }

        // ȣ��Ʈ ���� ��������
        if ((hostinf = gethostbyname(hostname)) == NULL) {
            err_quit("host entry error");
        }

        // IP �ּ� �������� �� ���
        for (int i = 0; hostinf->h_addr_list[i] != 0; ++i) {
            ip = inet_ntoa(*(struct in_addr*)hostinf->h_addr_list[i]);
        }
    }
    else {
        ip = "127.0.0.1";
    }

    return ip;
}

// ���� �Լ����� ���� �߻� ��, ���� �޽����� �޽��� �ڽ��� ����ϰ� ���α׷��� �����ϴ� �Լ�
void Client::err_quit(char* msg)
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

// [�ٽ�] �����κ��� �����͸� �����ϴ� ������ �����ϴ� ������ �Լ�
DWORD __stdcall Client::ThreadRecv(LPVOID arg) {
    int retval;
    char buf[BUFSIZE + 1];
    Packet pk = Packet();
    SOCKET sock = (SOCKET)arg; // ���� �����忡�� ���޹��� ���� �ڵ�

    // �� ������ ���α׷��� ����� ������ ��� ����Ǹ� ������ ����մϴ�.
    while (1) {
        // recv(): �����κ��� �����͸� ���� ������ �� �������� ���(���ŷ)�մϴ�.
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            // ���� �� ���� �߻�
            err_display("recv()");
            break;
        }
        else if (retval == 0) {
            // ������ ������ ���������� ������
            printf("\n[�˸�] �������� ������ ������ϴ�.\n");
            break;
        }

        buf[retval] = '\0';
        pk.RecvMsg(buf);
        pk.GetData(buf);

        // ���� �����͸� ȭ�鿡 ���
        printf("[���� ������] %s\n", buf);
    }
    return 0;
}