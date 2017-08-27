#include <iostream>
#include <stdexcept>

#include "runner.hpp"

int main(int , char** )
{
    using namespace smatch;
    try {
        Engine en;
        Runner::run(en, std::cin, std::cout);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
