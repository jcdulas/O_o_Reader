#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <future>
#include <random>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;

// Fixed-seed random models give the same neutral records when serialized concurrently.

namespace
{

// A real of a random magnitude and sign.
std::string randomReal(std::mt19937_64& generator)
{
    std::uniform_real_distribution<double> mantissa(-9.0, 9.0);
    std::uniform_int_distribution<int> exponent(-6, 6);
    return std::to_string(mantissa(generator) * std::pow(10.0, exponent(generator)));
}

// One record: the name and the fields joined by ';'.
std::string record(const std::string& name, const std::vector<std::string>& fields)
{
    std::string text = name;
    for (const std::string& field : fields)
        text += ";" + field;
    return text + "\n";
}

std::string randomModel(std::mt19937_64& generator)
{
    std::uniform_int_distribution<int> gridCount(4, 400);
    std::uniform_int_distribution<int> percent(0, 99);
    const int grids = gridCount(generator);
    std::string text = "[model]\nIsoMat;1;2.1E+11;;0.3;7850.\nShellProp;1;1;0.01\nBarProp;2;1;0.01;1.E-8;1.E-8;2.E-8\n";
    for (int id = 1; id <= grids; ++id)
    {
        if (percent(generator) < 5)
            text += "# grid point " + std::to_string(id) + "\n";
        text += record("Node",
                       {std::to_string(id), "", randomReal(generator), randomReal(generator), randomReal(generator)});
    }
    std::uniform_int_distribution<int> node(1, grids);
    int eid = 1;
    for (int index = 0; index < grids; ++index, ++eid)
    {
        const int kind = percent(generator);
        if (kind < 40)
            text += record("ShellQuad4",
                           {std::to_string(eid), "1", std::to_string(node(generator)), std::to_string(node(generator)),
                            std::to_string(node(generator)), std::to_string(node(generator))});
        else if (kind < 70)
            text += record("ShellTria3", {std::to_string(eid), "1", std::to_string(node(generator)),
                                         std::to_string(node(generator)), std::to_string(node(generator))});
        else if (kind < 90)
            text += record("Bar2", {std::to_string(eid), "2", std::to_string(node(generator)),
                                       std::to_string(node(generator)), "0.", "", "0.", "1."});
        else
            text += record("PointForceLoad", {std::to_string(1 + kind % 3), std::to_string(node(generator)), "",
                                              randomReal(generator), "0.", "0.", "1."});
    }
    text += "SinglePointConstrList;1;123456;" + std::to_string(node(generator)) + ";Through;" + std::to_string(grids) +
            "\n";
    return text;
}

} // namespace

TEST_CASE("random models produce deterministic records during concurrent reads", "[model][core][property]")
{
    std::mt19937_64 generator(83);
    for (int modelIndex = 0; modelIndex < 12; ++modelIndex)
    {
        const Model reference = testing::readModelText(randomModel(generator));
        REQUIRE(reference.cardCount() > 4);
        const std::string expected = testing::writeModelText(reference);
        std::vector<std::future<std::string>> writers;
        for (int trial = 0; trial < 3; ++trial)
            writers.push_back(std::async(std::launch::async, [&] { return testing::writeModelText(reference); }));
        for (auto& writer : writers)
            REQUIRE(writer.get() == expected);
    }
}
