#include "stdafx.h"
#include "epollserver.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc >= 2) {
        EpollServer server;
        if (server._listen(argv[1])) {
            while (server.pulse()) {
                usleep(1000);
            }
        }
    } else {
        cout << "Usage: [port]" << endl;
    }
    return 0;
}