#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// One column of a displacement matrix (DVSHAP) or of the load sequences of an auxiliary model (DVBSHAP) and the factor
// it is scaled by: COLi and SFi.
struct ShapeTerm
{
    std::int32_t column = 0; // >= 1
    double scale = 1.0;

    [[nodiscard]] bool operator==(const ShapeTerm&) const noexcept = default;
};

// Both shape cards carry at most this many pairs (format reference: COL1 to COL3).
inline constexpr std::size_t MAX_SHAPE_TERMS = 3;

// The pairs in the `MAX_SHAPE_TERMS` pairs of fields from `firstField`. A pair with both fields blank is skipped; a
// scale with no column is an error, and a column with no scale has the scale 1.0.
[[nodiscard]] std::vector<ShapeTerm> readShapeTerms(FieldReader& reader, int firstField);

// Writes the pairs the way readShapeTerms reads them.
void writeShapeTerms(FieldWriter& writer, int firstField, const std::vector<ShapeTerm>& terms);

// The problem with the pairs of a card, if there is one: none, more than three, a column below 1, a scale that is not
// finite.
[[nodiscard]] std::optional<CardProblem> checkShapeTerms(const std::vector<ShapeTerm>& terms);

} // namespace oor::model
