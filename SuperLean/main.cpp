#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include "main.h"

// uncomment one
//#define WHAT_TO_RUN traditional
#define WHAT_TO_RUN awaitable
//#define WHAT_TO_RUN improved

static void PrintHelp(char programName[])
{
    printf("%s - Coroutine benchmarking\n", programName);
    printf("Options:\n");
    printf("   -s <ip address> Run in server mode, listening on the given ip address.\n");
    printf("   -c <ip address> Run in client mode, connecting to the given ip address.\n");
    printf("   -n <number> Total number of readers or writers.\n");
    printf("   -y sync (inline) completion.\n");
    printf("   -h              Print this help message.\n");
}

inline void InvalidArgumentExit(char programPath[])
{
    PrintHelp(programPath);
    std::exit(1);
}

void main(int argc, char* argv[])
{
    if (argc < 3) {
        InvalidArgumentExit((argv)[0]);
    }

    bool isServer = false;
    bool sync = false;
    int nCoroutines = 1;
    const char* ipAddrStr = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strlen(argv[i]) != 2)
            InvalidArgumentExit((argv)[0]);

        switch (argv[i][1])
        {
            case 's':
                isServer = true;
                if (argc <= i + 1)
                    InvalidArgumentExit((argv)[0]);
                ++i;
                ipAddrStr = argv[i];
            break;

            case 'c':
                isServer = false;
                if (argc <= i + 1)
                    InvalidArgumentExit((argv)[0]);
                ++i;
                ipAddrStr = argv[i];
            break;

            case 'n':
                if (argc <= i + 1)
                    InvalidArgumentExit((argv)[0]);
                ++i;
                nCoroutines = atoi(argv[i]);
                break;

            case 'y':
                sync = true;
            break;

            case 'h':
            default:
                PrintHelp((argv)[0]);
                std::exit(0);
            break;

        }
    }

	int64_t bytes = 1'000'000'000;

	printf("%I64dM ", bytes / 1'000'000);

    if (isServer)
        WHAT_TO_RUN::run_server(ipAddrStr, nCoroutines, bytes, sync);
    else
	    WHAT_TO_RUN::run_client(ipAddrStr, nCoroutines, bytes, sync);
}
