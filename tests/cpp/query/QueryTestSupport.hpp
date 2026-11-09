#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/plan/Query.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

// What the tests of the query module share: tables of random values, a way to run a query on a given number of threads,
// and the comparison of two tables value by value.

namespace oor::query::testing
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// A table of `count` rows:
//   id      Int32, 1 .. count in a random order
//   group   Int32, 0 .. 9
//   x       Float64, between -1 and 1, one in twenty NaN
//   y       Float64, between 0 and 10, no NaN
//   flag    Bool
//   name    Utf8, "A", "B", "C" or "D"
//   key     Int64, 1000 * group + a value below 1000
//   maybe   Int32 with a null one time in seven
struct RandomTable
{
    std::vector<std::int32_t> id;
    std::vector<std::int32_t> group;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::uint8_t> flag;
    std::vector<std::string> name;
    std::vector<std::int64_t> key;
    std::vector<std::int32_t> maybe;
    std::vector<std::uint8_t> maybeValid;
    std::shared_ptr<const QueryTable> table;
};

inline RandomTable makeRandomTable(std::size_t count, unsigned seed)
{
    std::mt19937 generator(seed);
    RandomTable result;
    result.id.resize(count);
    for (std::size_t row = 0; row < count; ++row)
        result.id[row] = static_cast<std::int32_t>(row + 1);
    std::shuffle(result.id.begin(), result.id.end(), generator);
    std::uniform_int_distribution<int> groups(0, 9);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);
    std::uniform_real_distribution<double> positive(0.0, 10.0);
    std::uniform_int_distribution<int> small(0, 999);
    std::uniform_int_distribution<int> coin(0, 1);
    std::uniform_int_distribution<int> letters(0, 3);
    std::uniform_int_distribution<int> twenty(0, 19);
    std::uniform_int_distribution<int> seven(0, 6);
    const char* names[] = {"A", "B", "C", "D"};
    for (std::size_t row = 0; row < count; ++row)
    {
        result.group.push_back(groups(generator));
        result.x.push_back(twenty(generator) == 0 ? NOT_A_NUMBER : unit(generator));
        result.y.push_back(positive(generator));
        result.flag.push_back(static_cast<std::uint8_t>(coin(generator)));
        result.name.emplace_back(names[letters(generator)]);
        result.key.push_back(1000 * result.group.back() + small(generator));
        result.maybe.push_back(small(generator));
        result.maybeValid.push_back(seven(generator) == 0 ? 0 : 1);
    }
    auto table = std::make_shared<QueryTable>();
    table->addColumn("id", QueryColumn::primitive(std::vector<std::int32_t>(result.id)));
    table->addColumn("group", QueryColumn::primitive(std::vector<std::int32_t>(result.group)));
    table->addColumn("x", QueryColumn::primitive(std::vector<double>(result.x)));
    table->addColumn("y", QueryColumn::primitive(std::vector<double>(result.y)));
    table->addColumn("flag", QueryColumn::boolean(result.flag));
    table->addColumn("name", QueryColumn::utf8(result.name));
    table->addColumn("key", QueryColumn::primitive(std::vector<std::int64_t>(result.key)));
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(result.maybeValid, nullCount);
    table->addColumn("maybe",
                     QueryColumn::primitive(std::vector<std::int32_t>(result.maybe)).withValidity(bits, nullCount));
    result.table = table;
    return result;
}

// Runs a query on `threads` threads (1: the calling thread alone).
inline std::shared_ptr<const QueryTable> runOn(const Query& query, std::size_t threads)
{
    base::ThreadPool pool(threads - 1);
    ExecutionContext context(&pool, nullptr, nullptr);
    return query.run(context);
}

inline bool sameColumn(const QueryColumn& left, const QueryColumn& right)
{
    if (left.type() != right.type() || left.length() != right.length() || left.width() != right.width() ||
        left.nullCount() != right.nullCount())
        return false;
    const auto length = static_cast<std::size_t>(left.length());
    for (std::size_t row = 0; row < length; ++row)
    {
        if (left.isNull(row) != right.isNull(row))
            return false;
    }
    switch (left.type())
    {
    case ColumnType::Bool:
        for (std::size_t row = 0; row < length; ++row)
        {
            if (left.boolAt(row) != right.boolAt(row))
                return false;
        }
        return true;
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        for (std::size_t row = 0; row < length; ++row)
        {
            if (!left.isNull(row) && left.textAt(row) != right.textAt(row))
                return false;
        }
        return true;
    case ColumnType::FixedList:
    case ColumnType::List:
        for (std::size_t row = 0; row < length; ++row)
        {
            if (left.listLength(row) != right.listLength(row))
                return false;
        }
        return sameColumn(*left.child(), *right.child());
    default:
        return left.values().bytes() == right.values().bytes() &&
               std::memcmp(left.values().data(), right.values().data(), left.values().bytes()) == 0;
    }
}

inline bool sameTable(const QueryTable& left, const QueryTable& right)
{
    if (left.columnCount() != right.columnCount() || left.rowCount() != right.rowCount())
        return false;
    for (std::size_t index = 0; index < left.columnCount(); ++index)
    {
        if (left.name(index) != right.name(index) || !sameColumn(left.column(index), right.column(index)))
            return false;
    }
    return true;
}

} // namespace oor::query::testing
