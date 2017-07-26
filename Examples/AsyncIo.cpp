std::future<int> tcp_reader(int total) 
{
    char buf[64 * 1024];
    int result = 0;

    auto conn = co_await Tcp::Connect("127.0.0.1", 1337);   
    do 
    {
        auto bytesRead = co_await conn.Read(buf, sizeof(buf));
        total -= bytesRead;
        result += std::count(buf, buf + bytesRead, 'c');
    } 
    while (total > 0 && bytesRead > 0);
    co_return result;
}

int main() { tcp_reader(1000 * 1000 * 1000).get(); } 