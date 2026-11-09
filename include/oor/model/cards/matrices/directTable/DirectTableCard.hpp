#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// The kind of a value of a DTI record.
enum class DirectTableKind : std::uint8_t
{
    Blank,   // an empty field (kept: the position of a value in the record counts)
    Integer, // 105
    Real,    // 0.5
    Text     // arbitrary text
};

// One value of a DTI record: a field of the card, as written.
struct DirectTableValue
{
    DirectTableKind kind = DirectTableKind::Blank;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    std::string textValue; // as written

    [[nodiscard]] bool operator==(const DirectTableValue&) const noexcept = default;
};

// One DTI card: a record of a table data block that a DMAP reads (direct table input).
//
// Card layout:  DTI  NAME  IREC  V1  V2  V3  V4  V5  V6
//                                V7  V8  V9  V10  -etc.-  "ENDREC"
// NAME is the name of the data block (a word that starts with a letter), IREC the number of the record (0 is the
// optional header record, whose first six values T1 to T6 are the trailer). The values are the fields of the card from
// field 3 on, over as many continuation lines as needed: integers, reals, words or blanks, kept as written (the word
// ENDREC, sometimes used after blank values, is ordinary text too). The card does not know the layout
// of any data block; the library keeps the record and writes it back.
struct DirectTableCard
{
    std::string entryName; // upper case
    std::int32_t recordNumber = 0;
    std::vector<DirectTableValue> values;
};

} // namespace oor::model
