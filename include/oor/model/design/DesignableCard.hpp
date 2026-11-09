#pragma once

#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// What a design relation (DVPREL1/2, DVMREL1/2, DVCREL1/2) needs to know about the card it drives: which member of the
// card an exact neutral field descriptor identifies, how to read it, and how to write it. The knowledge is written by
// hand for each card type, next to the card (`<Card>DesignFields.cpp`), and the cards are listed in one place
// (`DesignableCards.cpp`). There is no table of the fields of several cards: a card type that has no function is a card
// the library cannot design, and a relation on it is refused with an error that names the card and the field.

namespace oor::model
{

// What the format reference says about the lowest value a relation may give a field when PMIN is blank: a DVPREL1,
// DVMREL1 or DVCREL1 gives 1.0E-15 for a field that can only be positive and -1.0E35 otherwise; a DVPREL2 gives 1.0E-15
// for every field but a stress recovery location, which is -1.0E35 (DVMREL2 and DVCREL2 follow the DVPREL1 rule).
// The solver prints these limits in the comparison of the analysis and design properties: the Z0 of a PCOMP, the Z1 and
// Z2 of a PSHELL and the F2 of a PBAR driven by a DVPREL2 have -1.0E35, the S1 of a PELAS (a stress coefficient, not a
// location) and every thickness, area and inertia 1.0E-15.
enum class DesignLowerLimit : std::uint8_t
{
    Positive,       // a thickness, an area, a modulus, a density: it can only be positive
    Signed,         // an offset, a ply angle, a product of inertia, a stress coefficient: it may be zero or negative
    StressRecovery, // a place where stress is recovered or the reference plane of a laminate: PSHELL Z1, Z2, PCOMP Z0,
                    // C1 to F2
};

// How much the mass of the elements depends on a field. The mass of the elements a variable drives is linear in the
// field for a thickness, an area, a density and a non-structural mass, so the sensitivity of the mass to a variable
// that drives them linearly is exact; a dimension of a section (PBARL, PBEAML, PTUBE) gives an area that is not linear
// in it.
enum class DesignMassEffect : std::uint8_t
{
    None,      // the field does not enter the mass (a stiffness, a stress location, a ply angle)
    Linear,    // the mass is a linear function of the field
    Nonlinear, // the mass depends on the field, not linearly
};

// A field of a card that a relation drives, as the functions of that card resolved it. `kind`, `index` and `station`
// are numbers that only the functions of the card give a meaning to (which member, which ply or dimension, which
// station of a beam); the rest is what the callers read.
struct DesignFieldFacts
{
    std::int32_t kind = 0;
    std::int32_t index = 0;
    std::int32_t station = 0;
    DesignLowerLimit lowerLimit = DesignLowerLimit::Signed;
    DesignMassEffect massEffect = DesignMassEffect::None;
    std::string name; // the name the format reference gives the field: "T", "A(B)", "DIM2(A)", "THETA3"
};

[[nodiscard]] inline DesignFieldFacts designFacts(std::int32_t kind, std::string name, DesignLowerLimit lowerLimit,
                                                  DesignMassEffect massEffect = DesignMassEffect::None,
                                                  std::int32_t index = 0, std::int32_t station = 0)
{
    DesignFieldFacts facts;
    facts.kind = kind;
    facts.index = index;
    facts.station = station;
    facts.lowerLimit = lowerLimit;
    facts.massEffect = massEffect;
    facts.name = std::move(name);
    return facts;
}

// The answer to "which field is this": the field, or why it is not one of the card.
struct DesignFieldLookup
{
    std::optional<DesignFieldFacts> facts;
    std::string problem;

    [[nodiscard]] static DesignFieldLookup found(DesignFieldFacts facts)
    {
        DesignFieldLookup result;
        result.facts = std::move(facts);
        return result;
    }
    [[nodiscard]] static DesignFieldLookup refused(std::string problem)
    {
        DesignFieldLookup result;
        result.problem = std::move(problem);
        return result;
    }
};

// Where a field is in the data contract (the `field` and `field_item` of `design_relations_table`): the name
// of the column of the card table that holds it, and, for a column that holds several values per card (a vector such as
// the C of a PBAR, or a list such as the ply thicknesses of a PCOMP or the station values of a PBEAM), the position of
// the field among the values of the card, flattened in row-major order (station * width + component); -1 for a column
// of one value per card.
struct DesignColumn
{
    std::string name;
    std::int32_t item = -1;
};

// A value for a field of a card. The facts are the ones `find` gave.
struct DesignFieldValue
{
    const DesignFieldFacts* facts = nullptr;
    double value = 0.0;
};

// The functions of one card type.
class DesignableCard
{
public:
    DesignableCard() = default;
    DesignableCard(const DesignableCard&) = delete;
    DesignableCard& operator=(const DesignableCard&) = delete;
    virtual ~DesignableCard() = default;

    [[nodiscard]] virtual std::string_view cardName() const noexcept = 0;

    // The field a name or a position is on the card in `row` (the answer may depend on the card: the number of plies
    // of a laminate, the dimensions of a section, the stations of a beam).
    [[nodiscard]] virtual DesignFieldLookup find(const Model& model, std::uint32_t row,
                                                 const DesignField& field) const = 0;

    // The value of the field the model holds.
    [[nodiscard]] virtual double read(const Model& model, std::uint32_t row, const DesignFieldFacts& facts) const = 0;

    // The contract column of the field on the card in `row`.
    [[nodiscard]] virtual DesignColumn column(const Model& model, std::uint32_t row,
                                              const DesignFieldFacts& facts) const = 0;

    // Gives the fields of the card in `row` their values, all of them at once: a field that holds a default which
    // depends on a field that was designed (the Z1 and Z2 of a PSHELL after T) follows it unless it is designed too.
    // The card is written only when a value differs.
    virtual void write(Model& model, std::uint32_t row, std::span<const DesignFieldValue> values) const = 0;
};

// The functions of a card type, or null for a card the library cannot design. The name is a card type, or the keyword
// the TYPE field of a design card writes (typeOfDesignedCard).
[[nodiscard]] const DesignableCard* findDesignableCard(std::string_view cardName);

// The names of the card types that have functions, in the order `DesignableCards.cpp` lists them.
[[nodiscard]] std::vector<std::string> designableCardNames();

} // namespace oor::model
