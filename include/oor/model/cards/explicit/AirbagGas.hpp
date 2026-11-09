#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <optional>
#include <string_view>

// What the airbag inflator cards (INFLCG and INFLTR) share, and no more: the one field that names the gas of the
// inflator. Each card still lays out, reads, writes and checks its own fields by hand.
namespace oor::model
{

// The GASNAM field of INFLCG (field 6) and INFLTR (field 5). It holds one of three things:
//   - a real: the ratio of the specific heats of the gas, and the card gives the gas constants itself;
//   - an integer: the number of an INFLGAS card that defines the gas, and the constants of the card stay blank;
//   - nothing: the card gives the gas constants itself, without the ratio.
// The format reference calls the field a name (a character) but INFLGAS is numbered, so a name is the number of an INFLGAS. A real
// is told from an integer by the decimal point, as everywhere in the deck: "1" is the INFLGAS 1, "1.0" is a ratio.
struct InflatorGasName
{
    std::optional<double> ratio; // the ratio of the specific heats, greater than zero
    base::EntityId gas = 0;     // an INFLGAS, 0 when the field holds no integer

    [[nodiscard]] bool operator==(const InflatorGasName&) const noexcept = default;
};

// Reads the field: a blank is nothing, a real is the ratio, an integer is the INFLGAS (which must be positive).
[[nodiscard]] InflatorGasName readInflatorGasName(FieldReader& reader, int field, const char* name);

void writeInflatorGasName(FieldWriter& writer, int field, const InflatorGasName& value);

// The problem with the value, if there is one: a ratio that is not greater than zero, an INFLGAS that is negative, or
// both at once (the field holds one or the other).
[[nodiscard]] std::optional<CardProblem> checkInflatorGasName(const InflatorGasName& value, const char* name);

// Visits the INFLGAS the field names, if it names one.
inline void visitInflatorGasName(ReferenceVisitor& visitor, std::string_view field, InflatorGasName& value)
{
    visitId(visitor, field, ReferenceTarget::InflatorGas, value.gas);
}

} // namespace oor::model
