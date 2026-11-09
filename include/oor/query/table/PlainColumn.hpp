#pragma once

#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::query
{

// What a column holds once turned into plain vectors.
enum class PlainKind
{
    Number,     // one number a row
    Text,       // one text a row
    NumberList, // a list of numbers a row (the nodes of an element)
    TextList    // a list of texts a row
};

// The values of one column of a query table in plain vectors: what the exports that do not speak Arrow (HDF5,
// JavaScript) need. A null number is NaN, a null text is empty. 64-bit integers become doubles: exact up to 2^53, far
// above any ID (IDs are 32-bit).
struct PlainColumn
{
    std::string name;
    PlainKind kind = PlainKind::Number;
    bool integer = false;              // the numbers are whole (an integer or boolean column)
    std::vector<double> numbers;       // Number, and the values of NumberList
    std::vector<std::string> texts;    // Text, and the values of TextList
    std::vector<std::int64_t> offsets; // the lists: row i holds the values [offsets[i], offsets[i + 1])
    std::vector<std::uint8_t> valid;   // 1 a row that has a value, 0 a null; empty when the column has no null
};

[[nodiscard]] PlainColumn plainColumn(const QueryTable& table, std::size_t index);
// The same for a column on its own, named `name`.
[[nodiscard]] PlainColumn plainColumn(const QueryColumn& column, std::string name);

} // namespace oor::query
