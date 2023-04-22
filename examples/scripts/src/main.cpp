#include <iostream>

#ifndef SPORE_CODEGEN
#include "main.scripts.hpp"
#endif

int main()
{
    std::cout << spore::codegen::examples::scripts::hello << std::endl;
    return 0;
}