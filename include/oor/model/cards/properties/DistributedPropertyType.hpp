#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

// What the two cards that distribute the data of a property over elements (PDISTB and PDISTBM) have in common: the
// word that names the kind of property, and the rule that an element is listed once. Each card still lays out, reads,
// writes and checks its own fields.
namespace oor::model
{

// The TYPE field: the name of the property card whose data the distribution replaces. The enumerators are numbered in
// the order of DISTRIBUTED_PROPERTY_WORDS, which holds the neutral card names, as a record writes them.
enum class DistributedPropertyType : std::uint8_t
{
    BarProp,
    BeamProp,
    BeamProp3,
    CompProp,
    CompPropGlobal,
    RodProp,
    ShearPanelProp,
    ShellProp,
    SolidProp,
    TubeProp
};

inline constexpr std::array<std::string_view, 10> DISTRIBUTED_PROPERTY_WORDS{
    "BarProp", "BeamProp",       "BeamProp3", "CompProp",  "CompPropGlobal",
    "RodProp", "ShearPanelProp", "ShellProp", "SolidProp", "TubeProp"};

// The problem with the elements one distribution card lists, if there is one: each must be a positive ID, and none may
// come twice (the format makes an element that two entries of the same property give a value
// to a fatal error; the entries of one card are the part of that rule a card can see by itself).
[[nodiscard]] inline std::optional<CardProblem> checkDistributedElements(std::vector<base::EntityId> elements)
{
    for (const base::EntityId element : elements)
    {
        if (element <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "elemId"};
    }
    std::sort(elements.begin(), elements.end());
    if (std::adjacent_find(elements.begin(), elements.end()) != elements.end())
        return CardProblem{
            "card.bad-value",
            "an element is listed twice: an " + std::string(CardProblem::FIELD) + " gives its value once", "elemId"};
    return std::nullopt;
}

} // namespace oor::model
