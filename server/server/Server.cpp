#include "Server.h"

vector<SOCKETINFO*> clients;    // ������ ��� Ŭ���̾�Ʈ�� SOCKETINFO �����͸� �����ϴ� ����
mutex clients_mutex;            // ���� �����尡 'clients' ���Ϳ� ���ÿ� �����ϴ� ���� �����ϱ� ���� ���ؽ�

Server::Server()
{
    // 1. Winsock �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
}

Server::~Server()
{
    WSACleanup();
}

void Server::Ready()
{
    // 2. IOCP ����
    // CreateIoCompletionPort: ���ο� IOCP�� �����ϰų� ���� IOCP�� �ڵ��� �����մϴ�.
    // ù ���ڰ� INVALID_HANDLE_VALUE�̸� ���ο� IOCP�� �����մϴ�.
    // ������ ����(NumberOfConcurrentThreads)�� 0���� �����ϸ� �ý����� CPU �ھ� ����ŭ�� �����带 ���ÿ� �����ϵ��� ����մϴ�.
    hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hcp == NULL) return;

    // 3. Worker Thread ����
    SYSTEM_INFO si;
    GetSystemInfo(&si); // �ý��� ����(CPU �ھ� �� ��)�� ����ϴ�.
    // �Ϲ������� CPU �ھ� ���� 2�踸ŭ Worker Thread�� �����ϴ� ���� ���� ������ ���Դϴ�.
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
        HANDLE hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
        CloseHandle(hThread); // ������ �ڵ��� �� �̻� �ʿ� �����Ƿ� �ٷ� �ݽ��ϴ�. (������ ��ü�� ��� �����)
    }

    // 4. ���� ���� ���� �� ����
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    SOCKADDR_IN serveraddr = { 0 };
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // ��� IP �ּҷκ����� ������ ����մϴ�.
    serveraddr.sin_port = htons(SERVERPORT);
    if (bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
        err_quit("listen()");
}

void Server::Run()
{
    // 5. Ŭ���̾�Ʈ ���� ���� ����
    while (1) {
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            continue;
        }
        printf("Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // 6. Ŭ���̾�Ʈ ������ IOCP�� ����
        // ������ Ŭ���̾�Ʈ ����(client_sock)�� ���� IOCP(hcp)�� �����մϴ�.
        // �� ��° ����(CompletionKey)�� ���� �ڵ��� �����Ͽ�, � ���Ͽ��� I/O�� �Ϸ�Ǿ����� �ĺ��� �� �ְ� �մϴ�.
        CreateIoCompletionPort((HANDLE)client_sock, hcp, (ULONG_PTR)client_sock, 0);

        // 7. Ŭ���̾�Ʈ ���� ����ü(SOCKETINFO) ���� �� �ʱ�ȭ
        SOCKETINFO* ptr = new SOCKETINFO;
        ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
        ptr->sock = client_sock;
        ptr->recvbytes = ptr->sendbytes = 0;
        ptr->wsabuf.buf = ptr->buf;
        ptr->wsabuf.len = BUFSIZE;
        ptr->sending = false; // �ʱ� ���´� �۽� ���� �ƴ�

        // 8. Ŭ���̾�Ʈ ��Ͽ� �߰� (Thread-safe)
        {
            lock_guard<mutex> lock(clients_mutex); // ���ؽ��� �̿��� 'clients' ���� ������ ��ȣ�մϴ�.
            clients.push_back(ptr);
        }

        // 9. �񵿱� ������ �߽�(WSASend) ��û
        // Ŭ���̾�Ʈ���� ȯ�� �޽���(�ð��� IP �ּ� ����)�� �񵿱������� �۽��� �����մϴ�.
        // �� send �Լ��� ���������� WSASend�� ȣ���ϸ�, �Ϸ�Ǹ� IOCP�� ���� �˸��� �ɴϴ�
        //string m_str;
        char m_buf[BUFSIZE + 1];
        time_t timer = time(NULL);
        struct tm* t = localtime(&timer);
        sprintf(m_buf, "%d�� %d�� %d�� %d�� %d�� %d�� %s, connection completed", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, inet_ntoa(clientaddr.sin_addr));

        memcpy(ptr->buf, m_buf, strlen(m_buf));
        ptr->recvbytes = strlen(m_buf);
        ptr->sendbytes = 0;
        ptr->sending = true;

        send(ptr);
    }
}

// Worker Thread: IOCP���� �Ϸ�� I/O �۾��� ó���ϴ� ������
DWORD WINAPI Server::WorkerThread(LPVOID arg) {
    HANDLE hcp = (HANDLE)arg;         // main �����忡�� ���޹��� IOCP �ڵ�
    DWORD cbTransferred;              // I/O �۾����� ���۵� ����Ʈ ��
    SOCKET client_sock;               // I/O�� �Ϸ�� ���� (Completion Key)
    SOCKETINFO* ptr;                  // I/O�� �Ϸ�� ������ �� ���� ����ü (Overlapped ������)
    int retval;

    ServerPacket pk = ServerPacket();

    while (1) {
        // GetQueuedCompletionStatus: IOCP ť���� �Ϸ�� I/O �۾��� ���� ������ ����մϴ�.
        // I/O�� �Ϸ�Ǹ�, �ش� �۾��� ���� ����(���۵� ����Ʈ, CompletionKey, OVERLAPPED ������)�� ��ȯ�մϴ�.
        retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (PULONG_PTR)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

        // Ŭ���̾�Ʈ ���� ���� ó��
        if (retval == 0 || cbTransferred == 0) {
            // GetQueuedCompletionStatus�� 0�� ��ȯ�ϰų� ���۵� ����Ʈ�� 0�̸� Ŭ���̾�Ʈ ������ ������ ���Դϴ�.
            if (ptr != NULL) { // ptr�� NULL�� ��쵵 ���� �� �����Ƿ� Ȯ��
                err_display("Client Disconnected");
                closesocket(ptr->sock);
                remove_client(ptr); // Ŭ���̾�Ʈ ��Ͽ��� ����
                ptr->sock = INVALID_SOCKET; // ������ ����
            }
            continue;
        }

        // IOCP �̺�Ʈ���� ptr�� NULL�̰ų� INVALID_SOCKET�̸� ����
        if (ptr && ptr->sock == INVALID_SOCKET) {
            delete ptr; // �Ҵ�� �޸� ����
            continue;
        }

        // I/O �۾��� ����(�۽�/����)�� ���� ó�� �б�
        if (ptr->sending) { // �۽� �۾� �Ϸ�
            ptr->sendbytes += cbTransferred;
            // ���� �����Ͱ� ���� �����ִ� ��� (�����Ͱ� �� ���� �� �������� ���� ���)
            if (ptr->sendbytes < ptr->recvbytes) {
                send(ptr); // ���� �����͸� ���� �����ϴ�.
            }
            else {
                // ���� �޽��� �۽� �Ϸ�
                ptr->sending = false; // �۽� ���� �÷��׸� �����ϴ�.

                // �۽� ��⿭(sendQueue)�� �ٸ� �޽����� �ִ��� Ȯ��
                if (!ptr->sendQueue.empty()) {
                    string next_msg = ptr->sendQueue.front();
                    ptr->sendQueue.pop();

                    memcpy(ptr->buf, next_msg.c_str(), next_msg.size());
                    ptr->recvbytes = next_msg.size(); // ���� ����Ʈ �� ����
                    ptr->sendbytes = 0; // ���� ����Ʈ �� �ʱ�ȭ
                    ptr->sending = true; // �ٽ� �۽� ���·� ��ȯ
                    send(ptr); // ���� �޽��� �۽� ����
                }
                else {
                    // ���� �޽����� �� ������, �ٽ� ������ ���� ��� ���·� ��ȯ�մϴ�.
                    receive(ptr);
                }
            }
        }
        else { // ���� �۾� �Ϸ�
            ptr->recvbytes = cbTransferred;
            ptr->buf[cbTransferred] = '\0'; // ���ڿ� ó���� ���� NULL ���� �߰�

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
                sprintf(m_buf, "%d�� %d�� %d�� %d�� %d�� %d�� %s:%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                pk.SetClose(m_buf);
            }
            else if (req_move == pk.GetType()) {
                pk.SendAllMove();
            }
            else if (chat_string == pk.GetType()) {
                pk.SChatMsg(ptr->name);
            }

            pk.GetBuf(m_buf);

            // ���� �޽����� �ٸ� ��� Ŭ���̾�Ʈ���� ���� (��ε�ĳ��Ʈ)
            broadcast(ptr, m_buf, pk.GetSize());

            // ���� �޽����� �ޱ� ���� �ٽ� �񵿱� ������ ��û�մϴ�.
            // �̰��� �񵿱� ���� �ٽ��Դϴ�. �ϳ��� �۾��� ������ ���� �۾��� �ٷ� �ٽð̴ϴ�.
            receive(ptr);
        }
    }
    return 0;
}

// �ڽ��� ������ ��� Ŭ���̾�Ʈ���� �޽����� �����ϴ� �Լ�
void Server::broadcast(SOCKETINFO* sender, const char* msg, int len) {
    lock_guard<mutex> lock(clients_mutex); // clients ���� ��ȣ
    for (auto client : clients) {
        if (client->sock == sender->sock) continue; // �޽����� ���� Ŭ���̾�Ʈ�� ����

        // �� Ŭ���̾�Ʈ�� �۽� ť�� �޽����� �߰��մϴ�.
        string message(msg, len);
        client->sendQueue.push(message);

        // ���� �ش� Ŭ���̾�Ʈ�� ���� �ٸ� �޽����� ������ ���� �ʴٸ�,
        // ��� �۽� �۾��� �����մϴ�.
        if (!client->sending) {
            string next_msg = client->sendQueue.front();
            client->sendQueue.pop();

            memcpy(client->buf, next_msg.c_str(), next_msg.size());
            client->recvbytes = next_msg.size(); // ���� ������ ũ��
            client->sendbytes = 0; // ���� ������ ũ�� �ʱ�ȭ
            client->sending = true; // �۽� ���·� ��ȯ
            send(client); // �񵿱� �۽� ����
        }
        // ���� 'sending'�� true���, ���� ���� ���� �۽��� ���� �� WorkerThread�� ť���� ���� �޽����� ���� ���� ���Դϴ�.
    }
}

// Ŭ���̾�Ʈ ��Ͽ��� Ư�� Ŭ���̾�Ʈ�� �����ϴ� �Լ�
void Server::remove_client(SOCKETINFO* ptr) {
    lock_guard<mutex> lock(clients_mutex); // clients ���� ��ȣ
    // C++ STL�� erase-remove idiom�� ����Ͽ� Ư�� ���Ҹ� �����մϴ�.
    clients.erase(remove(clients.begin(), clients.end(), ptr), clients.end());
}

// �񵿱� ������ �۽� �Լ� (WSASend ����)
bool Server::send(SOCKETINFO* ptr) {
    ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
    // ���� �������� ���� ��ġ�� ���̸� �����մϴ�. (���� ������ ����)
    ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
    ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;
    DWORD sendbytes;

    int retval = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0, &ptr->overlapped, NULL);
    // WSASend ȣ���� ����������, ���� �ڵ尡 WSA_IO_PENDING�� ���� �����Դϴ�.
    // �̴� I/O �۾��� ��׶��忡�� ���� ������ �ǹ��մϴ�.
    if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        err_display("WSASend()");
        return true; // ���� ���� �߻�
    }
    return false;
}

// �񵿱� ������ ���� �Լ� (WSARecv ����)
bool Server::receive(SOCKETINFO* ptr) {
    ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
    ptr->wsabuf.buf = ptr->buf;
    ptr->wsabuf.len = BUFSIZE;
    DWORD recvbytes, flags = 0;

    int retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
    // WSARecv ȣ���� ����������, ���� �ڵ尡 WSA_IO_PENDING�� ���� �����Դϴ�.
    if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        err_display("WSARecv()");
        return true; // ���� ���� �߻�
    }
    return false;
}

// ġ������ ���� �߻� �� �޽��� �ڽ��� ���� ���α׷��� �����ϴ� �Լ�
void Server::err_quit(char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �߻� �� �ֿܼ� ���� �޽����� ����ϴ� �Լ�
void Server::err_display(char* msg) {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}