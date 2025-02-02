#include <boost/asio.hpp>
#include <iostream>
#include <syncstream>
#include <thread>

namespace net = boost::asio;

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

int main() {
    using osync = std::osyncstream;
    using namespace std::chrono;

    net::io_context io{2};
    const auto start = steady_clock::now();

    auto print = [start](char ch) {
        const auto t = duration_cast<duration<double>>(steady_clock::now() - start).count();
        osync(std::cout) << std::fixed << std::setprecision(6)
            << t << "> " << ch << ':' << std::this_thread::get_id() << std::endl;
    };

    net::post(io, [&io, print] {
        print('A');

        net::defer(io, [print] { print('B'); });
        net::defer(io, [print] { print('C'); });
        net::defer(io, [print] { print('D'); });

        // Засыпаем, чтобы дать шанс другим потокам сделать свою работу
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });

    RunWorkers(2, [&io]{ io.run(); });
}