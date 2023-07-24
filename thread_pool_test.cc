// thread_pool_test.cpp

#include <iostream>
#include <random>
#include "thread_pool_demo.h"

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rand_sec = std::bind(dist, mt);

void simulate_hard_computation() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand_sec()));
}

void multiply(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << " Thread id: " << std::this_thread::get_id() << std::endl;
}

void multiply_output(int &out, const int a, const int b) {
    simulate_hard_computation();
    out = a * b;
    std::cout << a << " * " << b << " = " << out << " Thread id: " << std::this_thread::get_id() << std::endl;
}

int multiply_return(const int a, const int b) {
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << " Thread id: " << std::this_thread::get_id() << std::endl;
    return res;
}

void test() {
    ThreadPoolDemo thread_pool(4);
    thread_pool.init();

    for (int i = 1; i < 4; ++i) {
        for (int j = 1; j < 11; ++j) {
            thread_pool.submit(multiply, i, j);
        }
    }

    int output = 0;
    auto future_1 = thread_pool.submit(multiply_output, std::ref(output), 5, 20);
    future_1.get();
    std::cout << "Last operation result is equals to " << output << std::endl;

    auto future_2 = thread_pool.submit(multiply_return, 13, 14);
    int res = future_2.get();
    std::cout << "Last operation result is equals to " << res << std::endl;

    thread_pool.close();
}

int main(void) 
{
    std::cout << "Test start ..." << std::endl;
    test();
    std::cout << "Test end ..." << std::endl;

    return 0;
}
