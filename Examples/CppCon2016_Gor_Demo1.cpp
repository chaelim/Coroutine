//
//  Compiler: cl /std:c++latest /await /EHsc /O2 /Ox /EHsc /DNDEBUG /Fe.\_build\ <Cpp FileName>
//

#include <cstdio>
#include <experimental/generator>
#include <numeric>  // for std::accumulate


using namespace std::experimental;

auto gen()
{
    for (int i = 0;; ++i) {
        co_yield i;
    }
}

auto take_until(generator<int>& g, int sentinel)
{
    for (auto v: g) {
        if (v == sentinel)
            break;
        co_yield v;
    }
}

int main()
{
    auto g = gen();
    auto t = take_until(g, 10);
    auto r = std::accumulate(t.begin(), t.end(), 0);
    //printf("%d", r);
    return r;
}