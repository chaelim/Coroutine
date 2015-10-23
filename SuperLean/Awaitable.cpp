#include "stdafx.h"

#include <experimental\resumable>
#include <future>
#include "PAL.h"

struct task {
	~task() {}
	struct promise_type {
		task get_return_object() { return task{}; }
		void return_void() {}
		bool initial_suspend() { return false; }
		bool final_suspend() { return false; }
	};
};

using namespace std::experimental;

namespace awaitable
{
	namespace detail
	{
		struct AwaiterBase : os_async_context
		{
			coroutine_handle<> resume;
			std::error_code err;
			int bytes;
		};

		void io_complete_callback(CompletionPacket& p)
		{
			auto me = static_cast<AwaiterBase*>(p.overlapped);
			me->err = std::error_code(p.error, std::system_category());
			me->bytes = (int)p.byteTransferred;
			me->resume();
		}
	}

	struct Tcp
	{
		struct Connection
		{
			OsTcpSocket sock;

            Connection()
            {
                sock.Bind(endpoint{});
                ThreadPool::AssociateHandle(sock.native_handle(),
                    &detail::io_complete_callback);
            }

			Connection(OsTcpSocket && sock) : sock(std::move(sock)) {}


			Connection(endpoint ep) {
				sock.Bind(ep);
				ThreadPool::AssociateHandle(sock.native_handle(),
					&detail::io_complete_callback);
			}

			auto Read(void* buf, int len)
			{
				struct awaiter : detail::AwaiterBase {
					void* buf;
					int len;
					Connection* me;

					awaiter(Connection* me, void* buf, int len) : me(me), buf(buf), len(len) {}

					bool await_ready() { return false; }
					int await_resume() { return len; }

					bool await_suspend(coroutine_handle<> h) {
						this->resume = h;
						auto error = me->sock.Receive(buf, len, this);
						if (error.value() == kSynchCompletion) {
							return false;
						}
						if (error) {
							panic_if(!!error, "read failed");
						}
						return true;
					}
				};
				return awaiter{ this, buf, len };
			}

			auto Write(void* buf, int len)
			{
				struct awaiter : detail::AwaiterBase {
					void* buf;
					int len;
					Connection* me;

					awaiter(Connection* me, void* buf, int len) : me(me), buf(buf), len(len) {}

					bool await_ready() { return false; }
					int await_resume() { return len; }

					bool await_suspend(coroutine_handle<> h) {
						this->resume = h;
						auto error = me->sock.Send(buf, len, this);
						if (error.value() == kSynchCompletion) {
							return false;
						}
						if (error) {
							h.destroy();
						}
						return true;
					}
				};
				return awaiter{ this, buf, len };
			}
		};

		struct Listener
		{
			OsTcpSocket sock;

			Listener(const char* ipAddrStr) {
				sock.Bind(endpoint{ ipAddrStr, 13 });
				ThreadPool::AssociateHandle(sock.native_handle(), &detail::io_complete_callback);
				sock.Listen();
			}

			auto Accept() {
				struct awaiter : detail::AwaiterBase {
					Listener * me;
					OsTcpSocket sock;
					endpoint_name_pair ep;

					awaiter(Listener* me) : me(me) {}
					bool await_ready() { return false; }
					Connection await_resume() { return Connection{ std::move(sock) }; }

					void await_suspend(coroutine_handle<> h) {
						this->resume = h;

						ThreadPool::AssociateHandle(sock.native_handle(),
							&detail::io_complete_callback);

						me->sock.Accept(sock.native_handle(), ep, this);
					}
				};
				return awaiter{ this };
			}
		};

		static auto Connect(const char* s, unsigned short port)
		{
			struct awaiter : detail::AwaiterBase {
                Connection conn;
				endpoint ep;

				awaiter(const char* s, unsigned short port) : ep(s, port) { }
				bool await_ready() { return false; }
                Connection await_resume() {
					panic_if(!!this->err, "Connect failed");
					return std::move(conn);
				}

				void await_suspend(coroutine_handle<> h) {
					this->resume = h;
					conn.sock.Connect(ep, this);
				}
			};
			return awaiter{ s, port };
		}
	};

    auto Repost() { 
	    struct repost : detail::AwaiterBase {
		    bool await_ready() { return false; }
		    void await_resume() {}
		    void await_suspend(coroutine_handle<> h) {
			    this->resume = h;
			    ThreadPool::Post(this, 0 /* nBytes */, &detail::io_complete_callback);
		    }
	    };
        return repost{};
    }

	auto TcpReader(const char* ipAddrStr, WorkTracker& trk, int64_t total) -> std::future<void> {
		char buf[ReaderSize];
		auto conn = await Tcp::Connect(ipAddrStr, 13);
		while (total > 0) {
			auto bytes = await conn.Read(buf, sizeof(buf));
			if (bytes == 0) break;
			total -= bytes;
		}
		trk.completed();
	}

	auto ServeClient(Tcp::Connection conn) -> std::future<void> {
		char buf[WriterSize];
		await Repost();
		for (;;) {
			await conn.Write(buf, sizeof(buf));
		}
	}

	auto Server(const char* ipaddrStr) -> std::future<void>
	{
		Tcp::Listener s(ipaddrStr);
		for (;;) {
			ServeClient(await s.Accept());
		}
	}


	void run_server(const char* ipaddrStr, int nWriters, uint64_t bytes, bool sync) {
		printf("awaitable writers %d sync %d ", nWriters, sync);

		ThreadPool q(nWriters * 2 + 8, sync);

		Server(ipaddrStr);

		os_sleep(60 * 60 * 1000);
	}

    void run_client(const char* ipaddrStr, int nReaders, uint64_t bytes, bool sync)
    {
        printf("awaitable readers %d sync %d ", nReaders, sync);

        ThreadPool q(nReaders * 2 + 8, sync);
        WorkTracker trk(nReaders);

        for (int i = 0; i < nReaders; ++i)
            TcpReader(ipaddrStr, trk, bytes);

        os_sleep(60 * 60 * 1000);
    }

}