//
//  Compiler: cl /await /Zi /Fe.\_build\ HelloWorld.cpp 
//

#include <cstdio>
#include <experimental/generator>

auto Hello()
{
    for (auto ch : "Hello, World\n")
        co_yield ch;
}


void main()
{
    for (auto ch : Hello())
        printf("%c\n", ch);
}

/*
If coroutine lifetime is fully enclosed in the lifetime of the calling function, then we can
1) elide allocation and use a temporary on the stack of the caller
2) replace indirect calls with direct calls and inline as appropriate:

For example:

auto hello(char const* p) {

   while (*p) yield *p++;

}

int main() {

   for (auto c : hello("Hello, world"))

      putchar(c);

}



Should produce the same code as if you had written:

int main() {

   auto p = "Hello, world";

   while (*p) putchar(*p++);

}
*/
