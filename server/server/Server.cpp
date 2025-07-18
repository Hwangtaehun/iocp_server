#include "Server.h"

vector<SOCKETINFO*> clients;    // 접속한 모든 클라이언트의 SOCKETINFO 포인터를 관리하는 벡터
mutex clients_mutex;            // 여러 스레드가 'clients' 벡터에 동시에 접근하는 것을 방지하기 위한 뮤텍스

Server::Server()
{
    // 1. Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
}

Server::~Server()
{
    WSACleanup();
}

void Server::Ready()
{
    // 2. IOCP 생성
    // CreateIoCompletionPort: 새로운 IOCP를 생성하거나 기존 IOCP에 핸들을 연결합니다.
    // 첫 인자가 INVALID_HANDLE_VALUE이면 새로운 IOCP를 생성합니다.
    // 마지막 인자(NumberOfConcurrentThreads)를 0으로 설정하면 시스템이 CPU 코어 수만큼의 스레드를 동시에 실행하도록 허용합니다.
    hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hcp == NULL) return;

    // 3. Worker Thread 생성
    SYSTEM_INFO si;
    GetSystemInfo(&si); // 시스템 정보(CPU 코어 수 등)를 얻습니다.
    // 일반적으로 CPU 코어 수의 2배만큼 Worker Thread를 생성하는 것이 좋은 성능을 보입니다.
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
        HANDLE hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
        CloseHandle(hThread); // 스레드 핸들은 더 이상 필요 없으므로 바로 닫습니다. (스레드 자체는 계속 실행됨)
    }

    // 4. 리슨 소켓 생성 및 설정
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    SOCKADDR_IN serveraddr = { 0 };
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP 주소로부터의 접속을 허용합니다.
    serveraddr.sin_port = htons(SERVERPORT);
    if (bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
        err_quit("listen()");
}

void Server::Run()
{
    // 5. 클라이언트 접속 수락 루프
    while (1) {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            continue;
        }
        printf("클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // 6. 클라이언트 소켓을 IOCP에 연결
        // 생성된 클라이언트 소켓(client_sock)을 기존 IOCP(hcp)와 연결합니다.
        // 세 번째 인자(CompletionKey)로 소켓 핸들을 전달하여, 어떤 소켓에서 I/O가 완료되었는지 식별할 수 있게 합니다.
        CreateIoCompletionPort((HANDLE)client_sock, hcp, (ULONG_PTR)client_sock, 0);

        // 7. 클라이언트 정보 구조체(SOCKETINFO) 생성 및 초기화
        SOCKETINFO* ptr = new SOCKETINFO;
        ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
        ptr->sock = client_sock;
        ptr->recvbytes = ptr->sendbytes = 0;
        ptr->wsabuf.buf = ptr->buf;
        ptr->wsabuf.len = BUFSIZE;
        ptr->sending = false; // 초기 상태는 송신 중이 아님

        // 8. 클라이언트 목록에 추가 (Thread-safe)
        {
            lock_guard<mutex> lock(clients_mutex); // 뮤텍스를 이용해 'clients' 벡터 접근을 보호합니다.
            clients.push_back(ptr);
        }

        // 9. 비동기 데이터 발신(WSASend) 요청
        // 클라이언트에게 환영 메시지(시간과 IP 주소 포함)를 비동기적으로 송신을 시작합니다.
        // 이 send 함수는 내부적으로 WSASend를 호출하며, 완료되면 IOCP를 통해 알림이 옵니다
        //string m_str;
        char m_buf[BUFSIZE + 1];
        time_t timer = time(NULL);
        struct tm* t = localtime(&timer);
        sprintf(m_buf, "%d년 %d월 %d일 %d시 %d분 %d초 %s, connection completed", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, inet_ntoa(clientaddr.sin_addr));

        memcpy(ptr->buf, m_buf, strlen(m_buf));
        ptr->recvbytes = strlen(m_buf);
        ptr->sendbytes = 0;
        ptr->sending = true;

        send(ptr);
    }
}

// Worker Thread: IOCP에서 완료된 I/O 작업을 처리하는 스레드
DWORD WINAPI Server::WorkerThread(LPVOID arg) {
    HANDLE hcp = (HANDLE)arg;         // main 스레드에서 전달받은 IOCP 핸들
    DWORD cbTransferred;              // I/O 작업으로 전송된 바이트 수
    SOCKET client_sock;               // I/O가 완료된 소켓 (Completion Key)
    SOCKETINFO* ptr;                  // I/O가 완료된 소켓의 상세 정보 구조체 (Overlapped 포인터)
    int retval;

    ServerPacket pk = ServerPacket();

    while (1) {
        // GetQueuedCompletionStatus: IOCP 큐에서 완료된 I/O 작업이 생길 때까지 대기합니다.
        // I/O가 완료되면, 해당 작업에 대한 정보(전송된 바이트, CompletionKey, OVERLAPPED 포인터)를 반환합니다.
        retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

        // 클라이언트 연결 종료 처리
        if (retval == 0 || cbTransferred == 0) {
            // GetQueuedCompletionStatus가 0을 반환하거나 전송된 바이트가 0이면 클라이언트 연결이 끊어진 것입니다.
            if (ptr != NULL) { // ptr이 NULL인 경우도 있을 수 있으므로 확인
                err_display("Client Disconnected");
                closesocket(ptr->sock);
                remove_client(ptr); // 클라이언트 목록에서 제거
                ptr->sock = INVALID_SOCKET; // 재접근 방지
            }
            continue;
        }

        // IOCP 이벤트에서 ptr이 NULL이거나 INVALID_SOCKET이면 삭제
        if (ptr && ptr->sock == INVALID_SOCKET) {
            delete ptr; // 할당된 메모리 해제
            continue;
        }

        // I/O 작업의 종류(송신/수신)에 따라 처리 분기
        if (ptr->sending) { // 송신 작업 완료
            ptr->sendbytes += cbTransferred;
            // 보낼 데이터가 아직 남아있는 경우 (데이터가 한 번에 다 보내지지 않은 경우)
            if (ptr->sendbytes < ptr->recvbytes) {
                send(ptr); // 남은 데이터를 마저 보냅니다.
            }
            else {
                // 현재 메시지 송신 완료
                ptr->sending = false; // 송신 상태 플래그를 내립니다.

                // 송신 대기열(sendQueue)에 다른 메시지가 있는지 확인
                if (!ptr->sendQueue.empty()) {
                    string next_msg = ptr->sendQueue.front();
                    ptr->sendQueue.pop();

                    memcpy(ptr->buf, next_msg.c_str(), next_msg.size());
                    ptr->recvbytes = next_msg.size(); // 보낼 바이트 수 설정
                    ptr->sendbytes = 0; // 보낸 바이트 수 초기화
                    ptr->sending = true; // 다시 송신 상태로 전환
                    send(ptr); // 다음 메시지 송신 시작
                }
                else {
                    // 보낼 메시지가 더 없으면, 다시 데이터 수신 대기 상태로 전환합니다.
                    receive(ptr);
                }
            }
        }
        else { // 수신 작업 완료
            ptr->recvbytes = cbTransferred;
            ptr->buf[cbTransferred] = '\0'; // 문자열 처리를 위해 NULL 종단 추가

            SOCKADDR_IN clientaddr;
            int addrlen = sizeof(clientaddr);
            getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

            char m_buf[BUFSIZE] = { 0 };
            memcpy(m_buf, ptr->buf, ptr->recvbytes);
            m_buf[ptr->recvbytes] = '\0';
            pk.RecvMsg(m_buf);
            pk.GetData(m_buf);

            printf("[Message from %s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), m_buf);

            if (req_con == pk.GetType()) {
                memcpy(ptr->name, pk.GetName(), sizeof(pk.GetName()));
                pk.SendAllConnect(m_buf);;
            }
            else if (req_dis == pk.GetType()) {
                time_t timer = time(NULL);
                struct tm* t = localtime(&timer);
                sprintf(m_buf, "%d년 %d월 %d일 %d시 %d분 %d초 %s:%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                pk.SetClose(m_buf);
            }
            else if (req_move == pk.GetType()) {
                pk.SendAllMove();
            }
            else if (chat_string == pk.GetType()) {
                pk.SChatMsg(ptr->name);
            }

            pk.GetBuf(m_buf);

            // 받은 메시지를 다른 모든 클라이언트에게 전송 (브로드캐스트)
            broadcast(ptr, m_buf, pk.GetSize());

            // 다음 메시지를 받기 위해 다시 비동기 수신을 요청합니다.
            // 이것이 비동기 모델의 핵심입니다. 하나의 작업이 끝나면 다음 작업을 바로 다시겁니다.
            receive(ptr);
        }
    }
    return 0;
}

// 자신을 제외한 모든 클라이언트에게 메시지를 전송하는 함수
void Server::broadcast(SOCKETINFO* sender, const char* msg, int len) {
    lock_guard<mutex> lock(clients_mutex); // clients 벡터 보호
    for (auto client : clients) {
        if (client->sock == sender->sock) continue; // 메시지를 보낸 클라이언트는 제외

        // 각 클라이언트의 송신 큐에 메시지를 추가합니다.
        string message(msg, len);
        client->sendQueue.push(message);

        // 만약 해당 클라이언트가 현재 다른 메시지를 보내고 있지 않다면,
        // 즉시 송신 작업을 시작합니다.
        if (!client->sending) {
            string next_msg = client->sendQueue.front();
            client->sendQueue.pop();

            memcpy(client->buf, next_msg.c_str(), next_msg.size());
            client->recvbytes = next_msg.size(); // 보낼 데이터 크기
            client->sendbytes = 0; // 보낸 데이터 크기 초기화
            client->sending = true; // 송신 상태로 전환
            send(client); // 비동기 송신 시작
        }
        // 만약 'sending'이 true라면, 현재 진행 중인 송신이 끝난 후 WorkerThread가 큐에서 다음 메시지를 꺼내 보낼 것입니다.
    }
}

// 클라이언트 목록에서 특정 클라이언트를 제거하는 함수
void Server::remove_client(SOCKETINFO* ptr) {
    lock_guard<mutex> lock(clients_mutex); // clients 벡터 보호
    // C++ STL의 erase-remove idiom을 사용하여 특정 원소를 제거합니다.
    clients.erase(remove(clients.begin(), clients.end(), ptr), clients.end());
}

// 비동기 데이터 송신 함수 (WSASend 래퍼)
bool Server::send(SOCKETINFO* ptr) {
    ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
    // 보낼 데이터의 시작 위치와 길이를 설정합니다. (분할 전송을 위해)
    ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
    ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;
    DWORD sendbytes;

    int retval = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0, &ptr->overlapped, NULL);
    // WSASend 호출이 실패했지만, 오류 코드가 WSA_IO_PENDING인 경우는 정상입니다.
    // 이는 I/O 작업이 백그라운드에서 진행 중임을 의미합니다.
    if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        err_display("WSASend()");
        return true; // 실제 오류 발생
    }
    return false;
}

// 비동기 데이터 수신 함수 (WSARecv 래퍼)
bool Server::receive(SOCKETINFO* ptr) {
    ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
    ptr->wsabuf.buf = ptr->buf;
    ptr->wsabuf.len = BUFSIZE;
    DWORD recvbytes, flags = 0;

    int retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
    // WSARecv 호출이 실패했지만, 오류 코드가 WSA_IO_PENDING인 경우는 정상입니다.
    if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        err_display("WSARecv()");
        return true; // 실제 오류 발생
    }
    return false;
}

// 치명적인 오류 발생 시 메시지 박스를 띄우고 프로그램을 종료하는 함수
void Server::err_quit(char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// 오류 발생 시 콘솔에 오류 메시지를 출력하는 함수
void Server::err_display(char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}