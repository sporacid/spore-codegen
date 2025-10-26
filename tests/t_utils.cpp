#include "catch2/catch_all.hpp"

#include "spore/codegen/utils/strings.hpp"

TEST_CASE("spore::codegen::strings", "[spore::codegen][spore::codegen::strings]")
{
    using namespace spore::codegen;

    SECTION("split into words")
    {
        constexpr std::string_view TITLE_CASE0 = "Title case example 0. Work!";
        constexpr std::string_view SNAKE_CASE0 = "snake_case__example_0_work";
        constexpr std::string_view SNAKE_CASE1 = "snake_case_example1_work";
        constexpr std::string_view SNAKE_CASE2 = "  _snake_case_example2_work";
        constexpr std::string_view CAMEL_CASE0 = "camelCaseExampleZeroWork";
        constexpr std::string_view CAMEL_CASE1 = "CamelCaseExample1Work";
        constexpr std::string_view CAMEL_CASE2 = "CAMELCaseExampleTwoWork";
        constexpr std::string_view CAMEL_CASE3 = "CamelCaseEXAMPLE3Work";
        constexpr std::string_view CAMEL_CASE4 = "somethingIDs";
        constexpr std::string_view CAMEL_CASE5 = "AbcdA";
        constexpr std::string_view MIXED_CASE0 = "GM_Something";

        const std::vector<std::string_view> title_case0_expected {"Title", "case", "example", "0", "Work"};
        CHECK(title_case0_expected == strings::split_into_words(TITLE_CASE0));

        const std::vector<std::string_view> snake_case0_expected {"snake", "case", "example", "0", "work"};
        CHECK(snake_case0_expected == strings::split_into_words(SNAKE_CASE0));

        const std::vector<std::string_view> snake_case1_expected {"snake", "case", "example1", "work"};
        CHECK(snake_case1_expected == strings::split_into_words(SNAKE_CASE1));

        const std::vector<std::string_view> snake_case2_expected {"snake", "case", "example2", "work"};
        CHECK(snake_case2_expected == strings::split_into_words(SNAKE_CASE2));

        const std::vector<std::string_view> camel_case0_expected {"camel", "Case", "Example", "Zero", "Work"};
        CHECK(camel_case0_expected == strings::split_into_words(CAMEL_CASE0));

        const std::vector<std::string_view> camel_case1_expected {"Camel", "Case", "Example1", "Work"};
        CHECK(camel_case1_expected == strings::split_into_words(CAMEL_CASE1));

        const std::vector<std::string_view> camel_case2_expected {"CAMEL", "Case", "Example", "Two", "Work"};
        CHECK(camel_case2_expected == strings::split_into_words(CAMEL_CASE2));

        const std::vector<std::string_view> camel_case3_expected {"Camel", "Case", "EXAMPLE3", "Work"};
        CHECK(camel_case3_expected == strings::split_into_words(CAMEL_CASE3));

        const std::vector<std::string_view> camel_case4_expected {"something", "IDs"};
        CHECK(camel_case4_expected == strings::split_into_words(CAMEL_CASE4));

        const std::vector<std::string_view> camel_case5_expected {"Abcd", "A"};
        CHECK(camel_case5_expected == strings::split_into_words(CAMEL_CASE5));

        const std::vector<std::string_view> mixed_case0_expected {"GM", "Something"};
        CHECK(mixed_case0_expected == strings::split_into_words(MIXED_CASE0));
    }

    SECTION("to case")
    {
        struct test_case
        {
            std::string_view text;
            std::function<std::string(std::string_view)> to_case;
        };

        const std::vector<test_case> simple_test_case = {
            {"simpleTestCase", strings::to_camel_case},
            {"SimpleTestCase", strings::to_upper_camel_case},
            {"simple_test_case", strings::to_snake_case},
            {"SIMPLE_TEST_CASE", strings::to_upper_snake_case},
            {"Simple Test Case", strings::to_title_case},
            {"Simple test case", strings::to_sentence_case},
        };

        const std::vector<test_case> number_test_case = {
            {"number1TestCase", strings::to_camel_case},
            {"Number1TestCase", strings::to_upper_camel_case},
            {"number1_test_case", strings::to_snake_case},
            {"NUMBER1_TEST_CASE", strings::to_upper_snake_case},
            {"Number1 Test Case", strings::to_title_case},
            {"Number1 test case", strings::to_sentence_case},
        };

        const std::vector<test_case> separated_number_test_case = {
            {"number_1_test_case", strings::to_snake_case},
            {"NUMBER_1_TEST_CASE", strings::to_upper_snake_case},
            {"Number 1 Test Case", strings::to_title_case},
            {"Number 1 test case", strings::to_sentence_case},
        };

        const auto all_test_cases = std::to_array<std::vector<test_case>>({
            simple_test_case,
            number_test_case,
            separated_number_test_case,
        });

        for (auto const& test_case_group : all_test_cases)
        {
            // From all to all cases in the same group.
            for (const test_case& to : test_case_group)
            {
                for (const test_case& from : test_case_group)
                {
                    CHECK(to.text == to.to_case(from.text));
                }
            }
        }
    }
}
