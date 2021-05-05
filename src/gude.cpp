#include "gude.h"

int main(int argc, char* argv[])
{
    gude::ServerManager* server = new gude::ServerManager("./configs");
    if (server->init(argc, argv))
        return server->run();

    return 0;
}
