#pragma once

#include "oor/model/cards/optimisation/DesignField.hpp"

#include <optional>
#include <string>
#include <string_view>

// Neutral descriptor selectors shared by the card-family design resolvers.

namespace oor::model
{

// A name followed by a positive item index: plyThickness(3) splits into plyThickness and 3.
struct NumberedName
{
    std::string word;
    int number = 0;
};
[[nodiscard]] NumberedName splitTrailingNumber(std::string_view name);

// A station selector after a field name: area(EndA), area(EndB) or area(3).
struct StationName
{
    std::string word;
    bool hasStation = false;
    bool atEndA = false;
    bool atEndB = false;
    int intermediate = 0; // one-based station index, including EndA as 1
};
// Nullopt for a name whose parentheses are not closed or hold anything but EndA, EndB or a positive number.
[[nodiscard]] std::optional<StationName> splitStation(std::string_view name);

// True when a field that depends on another field still holds its resolved default to twelve digits. The cards store
// such a field resolved,
// so that a blank and the value it stands for look the same; when the field it depends on is designed, the field
// follows it only if it was at its default before.
[[nodiscard]] bool holdsDefault(double stored, double expected) noexcept;

[[nodiscard]] std::string refusalOfField(std::string_view cardType, const DesignField& field,
                                         std::string_view designable);

} // namespace oor::model
