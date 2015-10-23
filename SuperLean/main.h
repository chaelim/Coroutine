#pragma once
#include <cstdint>

namespace improved
{
    void run_client(const char* ipAddrStr, int nReaders, uint64_t bytes, bool sync);
    void run_server(const char* ipAddrStr, int nWriters, uint64_t bytes, bool sync);
}

namespace traditional
{
	void run(int nReaders, uint64_t bytes, bool sync);
}

namespace awaitable
{
    void run_client(const char* ipAddrStr, int nReaders, uint64_t bytes, bool sync);
	void run_server(const char* ipAddrStr, int nWriters, uint64_t bytes, bool sync);
}

