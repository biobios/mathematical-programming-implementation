#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Hello, TSP World!" << std::endl;
    
    if (argc > 1) {
        std::cout << "Arguments: ";
        for (int i = 1; i < argc; ++i) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}
