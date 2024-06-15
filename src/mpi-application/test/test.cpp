//
// Created by Ricardo Evans on 2023/6/5.
//

#include <iostream>
#include "ns3/mpi-application.h"
#include "ns3/mpi-operation.h"
#include "ns3/mpi-socket.h"
#include "ns3/mpi-functions.h"

struct Test {
    int i;

    Test() = default;

    Test(int i) : i(i) {}

    Test(const Test &test) = delete;

    Test(Test &&test) = default;

    Test &operator=(const Test &test) = delete;

    Test &operator=(Test &&test) = default;
};

void f(Test &t) {
    std::cout << "l-value reference" << std::endl;
}

void f(Test &&t) {
    std::cout << "r-value reference" << std::endl;
}


ns3::MPIOperation<void> test3() {
    co_return;
}

ns3::MPIOperation <Test> test2() {
    co_return 2;
}

ns3::MPIOperation <Test> testMPISocket() {
    ns3::MPISocket socket{ns3::Ptr < ns3::Socket > {}};
    co_await socket.accept();
    co_return 1;
}

ns3::MPIOperation <Test> test() {
    std::cout << "test 1" << std::endl;
    co_await std::suspend_always{};
    std::cout << "test 2" << std::endl;
    co_await std::suspend_never{};
    std::cout << "test 3" << std::endl;
    f(co_await test2());
    f(std::move(co_await test2()));
    co_await test3();
    co_return 1;
}

enum DataType {
    CHAR,
    INTEGER,
};

template<typename T, typename ...U>
decltype(auto) typeMapping(DataType type, T &&f, U &&...u) {
    switch (type) {
        case CHAR:
            return f.template operator()<char>(std::forward<U>(u)...);
        case INTEGER:
            return f.template operator()<int>(std::forward<U>(u)...);
        default:
            throw std::runtime_error("type not supported for mapping");
    }
}

ns3::MPIOperation<void> typeMappingTest() {
    auto f = []<typename T>() {
        std::cout << typeid(T).name() << std::endl;
    };
    ns3::MPICommunicator c; // can not run, but should compile
    DataType type = INTEGER; // or anything you like
    ns3::MPIRankIDType rank = 666; // or anything you like
    co_await typeMapping(type, [&c, rank]<typename T>() {
        return c.Send<T>(ns3::FakePacket, rank);
    });
    typeMapping(CHAR, f); // should output c
    typeMapping(INTEGER, f); // should output i
}

int main() {
    std::string trace_path = "../scratch/rn/traces";
    std::string app = "LULESH/dumpi-2013.09.09.04.32.40-0000.bin";
    // ns3::MPIRankIDType rank_id = 0;
    std::queue<ns3::MPIFunction> q = ns3::parse_trace(trace_path + app);
    return 0;
}