#include <chrono>

int main() {
    const auto start = std::chrono::steady_clock::now();
    const auto finish = std::chrono::steady_clock::now();
    return finish < start ? 1 : 0;
}
