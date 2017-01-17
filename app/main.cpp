#include <iostream>
#include <stdexcept>

#include "engine.hpp"

int main(int , char** )
{
    using namespace smatch;
    try
    {
        Engine engine;
        engine.run(std::cin, std::cout);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
