//
//  Compile
//  cl /std:c++latest /await /Zi /Fe.\_build\ demo1.cpp
//

#include <windows.h>
#include <future>
#include <cstdio>

std::future<int> f()
{
    printf("%x: Hello\n", GetCurrentThreadId());
    printf("%x: Hello\n", GetCurrentThreadId());
    co_return 42;
}


void main()
{
    printf("%x: got: %d\n", GetCurrentThreadId(), f().get());
}
