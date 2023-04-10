#include <iostream>

#include "json_types.hpp"

using namespace spore::codegen::examples::json;

int main()
{
    nlohmann::json json1 = generic_pair {
        "key1",
        "value1",
        1,
    };

    nlohmann::json json2 = template_pair<std::vector<std::string>> {
        "key2",
        {"value2", "value3"},
        2,
    };

    std::cout << "generic_pair: " << json1.dump(2) << std::endl;
    std::cout << "template_pair: " << json2.dump(2) << std::endl;

    return 0;
}