#include "Client.h"

// ���� �Լ�: ���α׷��� ������
int main(int argc, char* argv[])
{
    char name[20];
    printf("�̸��� �Է����ּ���.");
    scanf("%s", &name);
    getchar(); 
    Client client = Client();
    client.Ready();
    client.Run(name);
    client.~Client();

    return 0;
}
