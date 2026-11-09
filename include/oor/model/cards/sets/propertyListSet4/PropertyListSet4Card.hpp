#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

enum class PropertyListSet4Type : std::uint8_t
{
    SolidProp,
    ShellProp,
    ShearPanelProp,
    BarProp,
    BeamProp,
    WeldProp
};

inline constexpr std::array<std::string_view, 6> PROPERTY_LIST_SET_TYPE_WORDS{
    "SolidProp", "ShellProp", "ShearPanelProp", "BarProp", "BeamProp", "WeldProp"};

// One SET4 card: a list of property IDs of one type, which an FTGDEF (the fatigue definition) names.
//
// Card layout: SET4  ID  CLASS  TYPE  ID1  ID2  ID3  ID4  ID5
//                   .   ID6  ID7  ID8  -etc.-
// CLASS is the word PROP, required and with no other value (it has no default); the card writes it
// and keeps no member for it. TYPE is the property type the IDs belong to. The list is kept as written in `ids`: single
// IDs and "first THRU last" ranges, in order. A THRU may not be the first or the last field of a line of the card
// (not in the fifth or ninth column of the first line, nor in the second or ninth of a continuation line):
// the writer sees to it, the reader does not mind where the word is, as for the other lists. The ID is a set ID in the
// space of the SET1, SET2 and SET3 cards (IdSpace::Group: the format reference calls it unique, and a rule between
// cards can say so).
struct PropertyListSet4Card
{
    base::EntityId id = 0;
    PropertyListSet4Type type = PropertyListSet4Type::SolidProp;
    std::vector<IdRun> ids;

    [[nodiscard]] bool operator==(const PropertyListSet4Card&) const noexcept = default;
};

} // namespace oor::model
