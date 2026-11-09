#pragma once

#include "oor/model/core/CardProblem.hpp"

#include <cmath>
#include <optional>
#include <span>
#include <string>

// What the multi-dimensional tables of SOL 400 (TABL3D0, TABL3D1, TABL3D2, TABLE3D) share: the rule of the three flags
// that describe an independent variable, the rule of a list of reals, and the line arithmetic of a card whose lists
// each start on a line of their own. Each helper moves one kind of value; every card still lays out, reads, writes and
// checks its own fields by hand.
namespace oor::model
{

// The EXTRP of a variable when the field is blank: the table is continued past both ends of its data.
inline constexpr int TABLE_DEFAULT_EXTRAPOLATION = 2;

// The fields of one line of a card in the numbering of the format reference: a continuation line starts at field 9, 17,
// 25, ...
inline constexpr int TABLE_FIELDS_PER_LINE = 8;

// The first field of the line that holds `field` when `field` is not the first of its line, else `field` itself. The
// lists of a TABL3D1 and a TABL3D2 are laid out this way: each list of X values starts on a new line, and so do the Y.
[[nodiscard]] constexpr int tableLineStartAtOrAfter(int field) noexcept
{
    return ((field - 1 + TABLE_FIELDS_PER_LINE - 1) / TABLE_FIELDS_PER_LINE) * TABLE_FIELDS_PER_LINE + 1;
}

// The problem with the KIND of an independent variable, if there is one. The format reference numbers the kinds from 1
// (time and its normalised forms, the increment, the coordinates, temperature, strains, stresses, state variables,
// ...); it says that some of them are not available in SOL 400, and does not say which numbers a deck may not use, so
// the card checks the sign only.
[[nodiscard]] inline std::optional<CardProblem> checkIndependentVariableKind(int kind, const char* field)
{
    if (kind <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer", field};
    return std::nullopt;
}

// The problem with an extrapolation flag: 1 holds the end values, 2 continues the table past both of its ends.
[[nodiscard]] inline std::optional<CardProblem> checkExtrapolationFlag(int extrapolation, const char* field)
{
    if (extrapolation != 1 && extrapolation != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", field};
    return std::nullopt;
}

// The problem with a smoothing flag: 0 uses the data as they are, 1 smooths them.
[[nodiscard]] inline std::optional<CardProblem> checkSmoothingFlag(int smoothing, const char* field)
{
    if (smoothing != 0 && smoothing != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", field};
    return std::nullopt;
}

// The problem with a list of reals, if there is one: a value that is not a finite number.
[[nodiscard]] inline std::optional<CardProblem> checkFiniteTableValues(std::span<const double> values,
                                                                       const char* field)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be finite numbers", field};
    }
    return std::nullopt;
}

} // namespace oor::model
