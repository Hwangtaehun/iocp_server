#include "Client.h"

// 메인 함수: 프로그램의 시작점
int main(int argc, char* argv[])
{
    char name[20];
    printf("이름을 입력해주세요.");
    scanf("%s", &name);
    getchar(); 
    Client client = Client();
    client.Ready();
    client.Run(name);
    client.~Client();

    return 0;
}
