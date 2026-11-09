#include "oor/results/core/LabelArray.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "oor/results/core/ResultData.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace oor::results;

namespace
{

// A block of items whose owner says when it goes.
struct Block
{
    std::vector<std::int32_t> items;
    std::shared_ptr<bool> released = std::make_shared<bool>(false);
    std::shared_ptr<const void> keeper;

    explicit Block(std::vector<std::int32_t> values) : items(std::move(values))
    {
        auto flag = released;
        keeper = std::shared_ptr<const void>(items.data(), [flag](const void*) { *flag = true; });
    }
};

} // namespace

TEST_CASE("a label array of its own reads and changes as a vector", "[results][labels]")
{
    LabelArray<std::int32_t> array = std::vector<std::int32_t>{3, 1, 2};
    CHECK_FALSE(array.isKept());
    CHECK(array.size() == 3);
    array.push_back(7);
    array[0] = 4;
    CHECK(array == std::vector<std::int32_t>{4, 1, 2, 7});
    array.insert(array.begin() + 1, 9);
    array.erase(array.end() - 1);
    CHECK(array.toVector() == std::vector<std::int32_t>{4, 9, 1, 2});
    LabelArray<std::int32_t> copy = array;
    copy.resize(1);
    CHECK(array.size() == 4);
    CHECK(copy == std::vector<std::int32_t>{4});
    array.assign(2, 5);
    CHECK(array == LabelArray<std::int32_t>{5, 5});
    std::vector<std::int32_t> released = array.release();
    CHECK(released == std::vector<std::int32_t>{5, 5});
    CHECK(array.empty());
}

TEST_CASE("a kept label array reads the block and copies it on its first change", "[results][labels]")
{
    Block block({10, 20, 30});
    LabelArray<std::int32_t> array = LabelArray<std::int32_t>::kept(block.items.data(), 3, block.keeper);
    CHECK(array.isKept());
    const LabelArray<std::int32_t>& read = array;
    CHECK(read.data() == block.items.data());
    CHECK(read[1] == 20);
    CHECK(std::span<const std::int32_t>(read).size() == 3);

    SECTION("a copy keeps the same block, and the block lives while an array has it")
    {
        LabelArray<std::int32_t> copy = array;
        CHECK(copy.isKept());
        CHECK(std::as_const(copy).data() == block.items.data());
        block.keeper.reset();
        array = std::vector<std::int32_t>{1};
        CHECK_FALSE(*block.released);
        copy.clear();
        CHECK(*block.released);
    }
    SECTION("a change copies the items into a vector of its own and never writes the block")
    {
        array.push_back(40);
        CHECK_FALSE(array.isKept());
        CHECK(array == std::vector<std::int32_t>{10, 20, 30, 40});
        array[0] = 11;
        CHECK(block.items == std::vector<std::int32_t>{10, 20, 30});
        block.keeper.reset();
        CHECK(*block.released); // the array no longer needs the block
    }
    SECTION("an insertion into a kept array lands where its iterator says")
    {
        array.insert(std::as_const(array).begin() + 1, 15);
        CHECK(array == std::vector<std::int32_t>{10, 15, 20, 30});
        CHECK(block.items == std::vector<std::int32_t>{10, 20, 30});
    }
    SECTION("a move takes the block")
    {
        LabelArray<std::int32_t> moved = std::move(array);
        CHECK(moved.isKept());
        CHECK(std::as_const(moved).data() == block.items.data());
        block.keeper.reset();
        CHECK_FALSE(*block.released);
    }
}

TEST_CASE("a table keeps the labels it is given without a copy", "[results][labels][memory]")
{
    Block ids({1, 1, 2, 2});
    Block nodes({11, 11, 21, 21});
    Block layers({1, 2, 1, 2});
    MemoryTableInput input;
    input.kind = ResultKind::Stress;
    input.elementName = "ShellQuad4";
    input.components = {"oxx"};
    input.steps = {std::nan("")};
    const std::vector<double> values = {1.0, 2.0, 3.0, 4.0};
    input.values = std::as_bytes(std::span<const double>(values));
    input.ids = LabelArray<std::int32_t>::kept(ids.items.data(), 4, ids.keeper);
    input.nodeIds = LabelArray<std::int32_t>::kept(nodes.items.data(), 4, nodes.keeper);
    input.layers = LabelArray<std::int32_t>::kept(layers.items.data(), 4, layers.keeper);
    std::shared_ptr<ResultTable> table = makeMemoryTable(std::move(input));
    const std::shared_ptr<const ResultData> data = table->data();
    CHECK(table->meta().layout == "corner"); // the grid points were read
    CHECK(data->ids.data() == ids.items.data());
    CHECK(data->nodeIds.data() == nodes.items.data());
    CHECK(data->layers.data() == layers.items.data());
    ids.keeper.reset();
    layers.keeper.reset();
    CHECK_FALSE(*ids.released);
    table.reset();
    CHECK_FALSE(*ids.released); // the data of the table still has them
    CHECK(data->layers[3] == 2);
}

TEST_CASE("a repeated place in kept labels is refused as in a vector", "[results][labels][memory]")
{
    Block ids({1, 1});
    MemoryTableInput input;
    input.kind = ResultKind::Stress;
    input.elementName = "Rod2";
    input.components = {"axial"};
    input.steps = {std::nan("")};
    const std::vector<double> values = {1.0, 2.0};
    input.values = std::as_bytes(std::span<const double>(values));
    input.ids = LabelArray<std::int32_t>::kept(ids.items.data(), 2, ids.keeper);
    const std::vector<std::string> problems = checkMemoryTable(input);
    REQUIRE(problems.size() == 1);
    CHECK(problems.front().find("given more than once: 1 (1 repeated row)") != std::string::npos);
}

TEST_CASE("a text reads empty at a code outside its dictionary", "[results][labels]")
{
    std::vector<std::uint32_t> block = {0, 1, 7};
    const ResultText text("elementName", {"ShellQuad4", "total"},
                          LabelArray<std::uint32_t>::kept(block.data(), block.size(), std::make_shared<int>(0)));
    CHECK(text.at(1) == "total");
    CHECK(text.at(2).empty());
    CHECK(text.values() == std::vector<std::string>{"ShellQuad4", "total", ""});
}
