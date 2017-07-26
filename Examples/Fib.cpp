//
//  Compile command:
//  cl /await /Zi /EHsc /Fe.\_build\ Fib.cpp
//

#include <experimental/generator>

#include <stdio.h>
//generator<int>
auto fib(int n)
{
    uint64_t a = 0, b = 1;
    while (n-- > 0) {
        yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

void main()
{
    for (auto v : fib(50))
        printf("%llu ", v);
    
}
