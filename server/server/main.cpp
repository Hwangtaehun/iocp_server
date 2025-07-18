#include "Server.h"

int main(int argc, char* argv[]) {
    Server server = Server();

    server.Ready();
    server.Run();
    server.~Server();

    return 0;
}