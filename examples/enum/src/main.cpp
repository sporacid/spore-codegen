#include <iostream>

#include "enum_types.hpp"

using namespace spore::codegen::examples::enums;

int main()
{
    std::cout << "my_enum::value1: " << to_string(my_enum::value1) << std::endl;
    std::cout << "my_enum::value2: " << to_string(my_enum::value2) << std::endl;
    std::cout << "my_enum::value3: " << to_string(my_enum::value3) << std::endl;
    std::cout << "my_enum::value4: " << to_string(my_enum::value4) << std::endl;

    return 0;
}