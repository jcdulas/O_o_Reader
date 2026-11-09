#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/IdIndex.hpp"
#include "oor/model/storage/RowBook.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <random>
#include <unordered_map>

using namespace oor::model;
using oor::base::CardKey;

TEST_CASE("a column of defaults allocates nothing", "[model][storage]")
{
    Column<double> thickness(0.0);
    for (int index = 0; index < 1000; ++index)
        thickness.push(0.0);
    REQUIRE(thickness.size() == 1000);
    REQUIRE_FALSE(thickness.isMaterialised());
    REQUIRE(thickness.get(500) == 0.0);
    thickness.set(10, 0.0);
    REQUIRE_FALSE(thickness.isMaterialised()); // setting the default stays free

    thickness.set(10, 2.5); // the first value that differs allocates
    REQUIRE(thickness.isMaterialised());
    REQUIRE(thickness.get(10) == 2.5);
    REQUIRE(thickness.get(11) == 0.0);
    REQUIRE(thickness.size() == 1000);
}

TEST_CASE("an all-default column behaves like a filled one", "[model][storage]")
{
    Column<std::int32_t> lazy(7);
    Column<std::int32_t> filled(7);
    for (int index = 0; index < 5; ++index)
    {
        lazy.push(7);
        filled.push(7);
    }
    REQUIRE_FALSE(filled.isMaterialised());
    (void)filled.view(); // a view materialises
    REQUIRE(filled.isMaterialised());
    Column<std::int32_t> exported(7);
    exported.push(7);
    const auto shared = exported.share(); // an export (the buffer Python and Arrow read) materialises too
    REQUIRE(exported.isMaterialised());
    REQUIRE((*shared)[0] == 7);
    REQUIRE(lazy.sameValues(filled));
    REQUIRE(filled.sameValues(lazy));
    lazy.push(8);
    REQUIRE_FALSE(lazy.sameValues(filled));
    lazy.resize(5);
    REQUIRE(lazy.sameValues(filled));
}

TEST_CASE("a view is a snapshot: an edit copies, the view keeps its values", "[model][storage]")
{
    Column<double> values(0.0);
    values.push(1.0);
    values.push(2.0);
    const auto view = values.share();
    values.set(0, 99.0); // the buffer is shared with the view: it is cloned
    REQUIRE((*view)[0] == 1.0);
    REQUIRE(values.get(0) == 99.0);
    const auto second = values.share();
    values.push(3.0); // appending also copies when shared
    REQUIRE(second->size() == 2);
    REQUIRE(values.size() == 3);
}

TEST_CASE("an edit without a live view copies nothing", "[model][storage]")
{
    Column<double> values(0.0);
    values.push(1.0);
    const double* before = values.view().data();
    values.set(0, 5.0);
    REQUIRE(values.view().data() == before); // same buffer: no copy
}

TEST_CASE("a copy of a column shares its buffer until one side edits", "[model][storage]")
{
    Column<std::int32_t> original(0);
    for (int index = 1; index <= 4; ++index)
        original.push(index);
    Column<std::int32_t> copy = original;
    REQUIRE(copy.view().data() == original.view().data());
    copy.set(1, 100);
    REQUIRE(original.get(1) == 2);
    REQUIRE(copy.get(1) == 100);
}

TEST_CASE("keepRows and append", "[model][storage]")
{
    Column<std::int32_t> values(0);
    for (int index = 0; index < 6; ++index)
        values.push(index * 10);
    values.keepRows({0, 2, 5});
    REQUIRE(values.size() == 3);
    REQUIRE(values.get(1) == 20);
    REQUIRE(values.get(2) == 50);

    Column<std::int32_t> tail(0);
    tail.push(7);
    tail.push(0);
    values.append(tail);
    REQUIRE(values.size() == 5);
    REQUIRE(values.get(3) == 7);

    Column<std::int32_t> defaults(0);
    defaults.push(0);
    defaults.push(0);
    Column<std::int32_t> alsoDefaults(0);
    alsoDefaults.push(0);
    defaults.append(alsoDefaults);
    REQUIRE(defaults.size() == 3);
    REQUIRE_FALSE(defaults.isMaterialised());
}

TEST_CASE("a variable length column keeps lists of any length", "[model][storage]")
{
    VariableLengthColumn<std::int32_t> nodes;
    const std::array<std::int32_t, 4> quad = {1, 2, 3, 4};
    const std::array<std::int32_t, 3> triangle = {5, 6, 7};
    nodes.push(quad);
    nodes.push(triangle);
    nodes.push({});
    REQUIRE(nodes.size() == 3);
    REQUIRE(nodes.get(0).size() == 4);
    REQUIRE(nodes.get(1)[2] == 7);
    REQUIRE(nodes.get(2).empty());

    // Replacing with a list of another length leaves a hole; canonicalise squeezes it out.
    const std::array<std::int32_t, 2> two = {8, 9};
    nodes.set(0, two);
    REQUIRE(nodes.get(0).size() == 2);
    REQUIRE(nodes.get(0)[1] == 9);
    nodes.canonicalise();
    REQUIRE(nodes.offsets() == std::vector<std::uint64_t>{0, 2, 5, 5});
    REQUIRE(nodes.values().size() == 5);
    REQUIRE(nodes.get(1)[0] == 5);

    nodes.keepRows({1, 2});
    REQUIRE(nodes.size() == 2);
    REQUIRE(nodes.get(0)[0] == 5);
}

TEST_CASE("a variable length column is copy on write too", "[model][storage]")
{
    VariableLengthColumn<std::int32_t> nodes;
    const std::array<std::int32_t, 2> pair = {1, 2};
    nodes.push(pair);
    VariableLengthColumn<std::int32_t> copy = nodes;
    const std::array<std::int32_t, 2> other = {9, 9};
    copy.set(0, other);
    REQUIRE(nodes.get(0)[0] == 1);
    REQUIRE(copy.get(0)[0] == 9);
}

TEST_CASE("rows are found by binary search on their keys and killed, then compacted", "[model][storage]")
{
    RowBook rows;
    for (std::uint32_t key : {3u, 7u, 8u, 20u, 21u})
        rows.append(CardKey{key});
    REQUIRE(rows.aliveCount() == 5);
    REQUIRE(rows.findRow(CardKey{8}) == 2);
    REQUIRE(rows.findRow(CardKey{9}) == -1);

    const std::uint64_t epochBefore = rows.epoch();
    rows.kill(1);
    rows.kill(3);
    REQUIRE(rows.epoch() > epochBefore);
    REQUIRE(rows.aliveCount() == 3);
    REQUIRE(rows.findRow(CardKey{7}) == 1); // dead rows keep their place until compaction
    REQUIRE(rows.findLiveRow(CardKey{7}) == -1);
    rows.kill(1); // killing twice is harmless
    REQUIRE(rows.aliveCount() == 3);

    const auto live = rows.liveRows();
    REQUIRE(live == std::vector<std::uint32_t>{0, 2, 4});
    rows.applyCompaction(live);
    REQUIRE(rows.rowCount() == 3);
    REQUIRE(rows.findRow(CardKey{7}) == -1); // gone: a handle on it is stale
    REQUIRE(rows.findLiveRow(CardKey{21}) == 2);
    REQUIRE(rows.keyAt(1).value == 8);
}

TEST_CASE("the ID index matches a reference map under random operations", "[model][storage][property]")
{
    IdIndex index;
    std::unordered_map<std::int32_t, std::uint32_t> reference;
    std::mt19937 generator(99);
    for (int step = 0; step < 40000; ++step)
    {
        const std::int32_t id = static_cast<std::int32_t>(generator() % 3000) - 100;
        switch (generator() % 3)
        {
        case 0:
        {
            const auto previous = index.insert(id, CardLocation{1, static_cast<std::uint32_t>(step)});
            REQUIRE(previous.has_value() == reference.contains(id));
            reference[id] = static_cast<std::uint32_t>(step);
            break;
        }
        case 1:
            REQUIRE(index.erase(id) == (reference.erase(id) > 0));
            break;
        default:
        {
            const auto found = index.find(id);
            REQUIRE(found.has_value() == reference.contains(id));
            if (found)
                REQUIRE(found->row == reference[id]);
        }
        }
        REQUIRE(index.size() == reference.size());
    }
    std::size_t visited = 0;
    index.forEach(
        [&](std::int32_t id, CardLocation location)
        {
            REQUIRE(reference.at(id) == location.row);
            ++visited;
        });
    REQUIRE(visited == reference.size());
}

TEST_CASE("the ID index matches a reference map when its IDs turn sparse", "[model][storage][property]")
{
    // Dense IDs first (the array form), then IDs spread over the whole range of int32: the index turns into a hash
    // table on the way and keeps every entry.
    IdIndex index;
    std::unordered_map<std::int32_t, std::uint32_t> reference;
    std::mt19937 generator(7);
    for (int step = 0; step < 20000; ++step)
    {
        const bool sparse = step >= 10000;
        const std::int32_t id =
            sparse ? static_cast<std::int32_t>(generator()) : static_cast<std::int32_t>(generator() % 1000) + 5000;
        if (generator() % 4 == 0)
        {
            REQUIRE(index.erase(id) == (reference.erase(id) > 0));
        }
        else
        {
            const auto previous = index.insert(id, CardLocation{3, static_cast<std::uint32_t>(step)});
            REQUIRE(previous.has_value() == reference.contains(id));
            reference[id] = static_cast<std::uint32_t>(step);
        }
        if (step == 9999)
            REQUIRE(index.isDense());
        REQUIRE(index.size() == reference.size());
    }
    REQUIRE_FALSE(index.isDense());
    for (const auto& [id, row] : reference)
        REQUIRE(index.find(id)->row == row);
    std::size_t visited = 0;
    index.forEach(
        [&](std::int32_t id, CardLocation location)
        {
            REQUIRE(reference.at(id) == location.row);
            ++visited;
        });
    REQUIRE(visited == reference.size());
}

TEST_CASE("IDs inserted in decreasing order and at the ends of int32 stay found", "[model][storage]")
{
    IdIndex index;
    for (std::int32_t id = 100000; id >= 1; --id)
        index.insert(id, CardLocation{1, static_cast<std::uint32_t>(id)});
    REQUIRE(index.isDense());
    REQUIRE(index.size() == 100000);
    REQUIRE(index.find(1)->row == 1);
    REQUIRE(index.find(100000)->row == 100000);
    REQUIRE_FALSE(index.find(0).has_value());
    REQUIRE_FALSE(index.find(100001).has_value());
    // A lone ID far away turns the index into a hash table.
    index.insert(std::numeric_limits<std::int32_t>::max(), CardLocation{1, 7});
    index.insert(std::numeric_limits<std::int32_t>::min(), CardLocation{1, 8});
    REQUIRE_FALSE(index.isDense());
    REQUIRE(index.size() == 100002);
    REQUIRE(index.find(std::numeric_limits<std::int32_t>::max())->row == 7);
    REQUIRE(index.find(std::numeric_limits<std::int32_t>::min())->row == 8);
    REQUIRE(index.find(54321)->row == 54321);
    index.clear();
    REQUIRE(index.isDense());
    REQUIRE(index.size() == 0);
    REQUIRE_FALSE(index.find(54321).has_value());
    // The smallest IDs of int32 in the array form.
    index.insert(std::numeric_limits<std::int32_t>::min() + 1, CardLocation{2, 1});
    index.insert(std::numeric_limits<std::int32_t>::min(), CardLocation{2, 0});
    REQUIRE(index.isDense());
    REQUIRE(index.find(std::numeric_limits<std::int32_t>::min())->row == 0);
    REQUIRE(index.find(std::numeric_limits<std::int32_t>::min() + 1)->row == 1);
}

TEST_CASE("sequential IDs fill the index without collisions", "[model][storage]")
{
    IdIndex index;
    index.reserve(100000);
    for (std::int32_t id = 1; id <= 100000; ++id)
        index.insert(id, CardLocation{0, static_cast<std::uint32_t>(id)});
    REQUIRE(index.size() == 100000);
    for (std::int32_t id : {1, 50000, 100000})
        REQUIRE(index.find(id)->row == static_cast<std::uint32_t>(id));
    REQUIRE_FALSE(index.find(100001).has_value());
}

TEST_CASE("a copy of an ID index shares its entries until one side changes them", "[model][storage]")
{
    for (const bool sparse : {false, true})
    {
        IdIndex original;
        for (std::int32_t id = 1; id <= 100; ++id)
            original.insert(sparse ? id * 1'000'000 : id, CardLocation{1, static_cast<std::uint32_t>(id)});
        REQUIRE(original.isDense() == !sparse);
        const std::int32_t scale = sparse ? 1'000'000 : 1;
        IdIndex copy = original;
        copy.insert(101 * scale, CardLocation{1, 101});
        REQUIRE(copy.erase(5 * scale));
        REQUIRE(copy.insert(7 * scale, CardLocation{2, 70}) == CardLocation{1, 7});
        // the original is as it was
        REQUIRE(original.size() == 100);
        REQUIRE_FALSE(original.find(101 * scale).has_value());
        REQUIRE(original.find(5 * scale)->row == 5);
        REQUIRE(original.find(7 * scale) == CardLocation{1, 7});
        REQUIRE(copy.size() == 100);
        REQUIRE_FALSE(copy.find(5 * scale).has_value());
        REQUIRE(copy.find(7 * scale) == CardLocation{2, 70});
        // and the other way round
        REQUIRE(original.erase(1 * scale));
        REQUIRE(copy.find(1 * scale)->row == 1);
        REQUIRE_FALSE(original.erase(999 * scale)); // nothing to remove
        original.clear();
        REQUIRE(copy.find(100 * scale)->row == 100);
    }
}

TEST_CASE("a copy of a row book shares its rows until one side kills, compacts or forgets one", "[model][storage]")
{
    RowBook original;
    const std::array<std::uint8_t, 3> digits{5, 0, 8};
    for (std::uint32_t key : {3u, 7u, 8u})
    {
        const std::uint32_t row = original.append(CardKey{key});
        original.setSource(row, digits);
    }
    RowBook copy = original;
    copy.kill(1);
    REQUIRE(original.isAlive(1));
    REQUIRE(original.aliveCount() == 3);
    REQUIRE_FALSE(copy.isAlive(1));
    copy.forgetSource(0);
    REQUIRE(copy.significantDigits(0).empty());
    REQUIRE(original.significantDigits(0).size() == 3);
    copy.applyCompaction(copy.liveRows());
    REQUIRE(copy.rowCount() == 2);
    REQUIRE(original.rowCount() == 3);
    REQUIRE(original.significantDigits(1)[2] == 8);
    REQUIRE(copy.significantDigits(1)[2] == 8); // key 8, now in row 1 of the copy
    original.kill(2);
    REQUIRE(copy.isAlive(1));
    REQUIRE(original.liveRows() == std::vector<std::uint32_t>{0, 1});
}

TEST_CASE("a column that grows during a batch grows once by the rows the batch announced", "[model][storage]")
{
    Column<std::int32_t> column(0);
    column.push(1); // allocated: a value that is not the default
    const std::size_t before = column.view().size();
    {
        const ColumnGrowth::Batch batch(10'000);
        for (std::int32_t value = 2; value <= 1'000; ++value)
            column.push(value);
        REQUIRE(ColumnGrowth::announced() == 10'000);
    }
    REQUIRE(ColumnGrowth::announced() == 0); // the batch is over
    REQUIRE(before == 1);
    REQUIRE(column.size() == 1'000);
    REQUIRE(column.get(999) == 1'000);
    // one allocation for the batch: the buffer did not move while the values were pushed
    const std::int32_t* data = column.view().data();
    {
        const ColumnGrowth::Batch batch(100);
        for (std::int32_t value = 0; value < 100; ++value)
            column.push(value);
    }
    REQUIRE(column.view().data() == data); // room was left by the first batch
}

TEST_CASE("a column keeps the sign of a zero", "[model][storage]")
{
    // a companion package: -0.0 is not the default 0.0 (to the bit), so that a lossless write gives it back
    Column<double> values;
    values.push(0.0);
    values.push(-0.0);
    REQUIRE(values.isMaterialised());
    REQUIRE_FALSE(std::signbit(values.get(0)));
    REQUIRE(std::signbit(values.get(1)));
    values.set(0, -0.0);
    REQUIRE(std::signbit(values.get(0)));

    Column<std::array<double, 3>> vectors;
    vectors.push({-0.0, 0.0, -0.0});
    REQUIRE(vectors.isMaterialised());
    REQUIRE(std::signbit(vectors.get(0)[0]));
    REQUIRE(std::signbit(vectors.get(0)[2]));
}
