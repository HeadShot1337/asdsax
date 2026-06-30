#include <iostream>

// Simple function to be virtualized
extern "C" int test_func(int a, int b) {
    int sum = a + b;
    int diff = a - b;
    return sum * diff;
}

int main() {
    int x = 10;
    int y = 5;
    std::cout << "Result: " << test_func(x, y) << std::endl;
    return 0;
}
