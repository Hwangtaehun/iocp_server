#include "Client.h"

int main(int argc, char* argv[])
{
    char name[20];
    printf("�̸��� �Է����ּ���.");
    scanf("%s", &name);
    getchar();
    name[19] = '\0';
    Client client = Client();
    client.Ready();
    client.Run(name);
    client.~Client();
    
    return 0;
}
