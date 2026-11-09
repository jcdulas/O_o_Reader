#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// What the ID of a design relation names, from the name of the card it drives: the PID of a DVPREL1 of type PBAR names
// a property, the MID of a DVMREL1 of type MAT1 a material, the EID of a DVCREL1 of type CQUAD4 an element, and the
// same holds for the PID of a DTABLE2 entry. The card types come from the registry, so a card that the library does not
// type (a property of another solver) names nothing it could resolve and gives no target: its ID is not looked up.
[[nodiscard]] std::optional<ReferenceTarget> targetOfDesignedCard(std::string_view cardName);

// The card type a design card names by its neutral card name ("ShellProp"), as the record writes it. Nullopt for a
// card the library does not type.
[[nodiscard]] std::optional<std::uint16_t> typeOfDesignedCard(std::string_view cardName);

// The check of a field that holds a neutral name (a card type, a word of the format): one or more letters and digits,
// the first a letter. The names of the code are longer than the eight characters of a deck name.
[[nodiscard]] std::optional<CardProblem> checkNeutralName(std::string_view name, const char* field);

} // namespace oor::model
