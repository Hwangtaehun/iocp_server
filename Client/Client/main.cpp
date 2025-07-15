#include "Client.h"

int main(int argc, char* argv[])
{
    char name[20];
    printf("이름을 입력해주세요.");
    scanf("%s", &name);
    getchar();
    name[19] = '\0';
    Client client = Client();
    client.Ready();
    client.Run(name);
    client.~Client();
    
    return 0;
}
