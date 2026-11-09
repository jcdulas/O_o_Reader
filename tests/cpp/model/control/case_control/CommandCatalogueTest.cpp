#include "oor/model/control/case_control/CommandCatalogue.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
using namespace oor::model;
TEST_CASE("commands are found by their exact neutral name", "[control][catalogue]")
{
    for (const char* name : {"DisplacementOutput", "StressOutput", "SinglePointConstrSelection", "TemperatureSelection",
                             "SubcaseStart", "TrimSelection", "SetDefinition"})
        CHECK(CommandCatalogue::instance().findByName(name) != nullptr);
}
TEST_CASE("unknown or differently cased command names are refused", "[control][catalogue]")
{
    for (const char* name : {"displacementOutput", "Displacement", "noSuchCommand"})
        CHECK(CommandCatalogue::instance().findByName(name) == nullptr);
}
TEST_CASE("a variant uses its exact neutral name", "[control][catalogue]")
{
    const auto* temperature = CommandCatalogue::instance().findByName("TemperatureSelection");
    REQUIRE(temperature != nullptr);
    CHECK(temperature->hasVariant("InitialTemperature"));
    CHECK(temperature->hasVariant("TemperatureLoadPart"));
    CHECK_FALSE(temperature->hasVariant("initialTemperature"));
}
TEST_CASE("the catalogue assigns each neutral command name once", "[control][catalogue]")
{
    const auto& catalogue = CommandCatalogue::instance();
    CHECK(catalogue.duplicateNames().empty());
    std::set<std::string> names;
    for (const auto& definition : catalogue.definitions())
        CHECK(names.insert(definition.name).second);
    CHECK(names.size() > 250);
}
