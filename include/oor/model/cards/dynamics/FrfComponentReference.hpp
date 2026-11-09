#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

// What the cards of an FRF based assembly (FBA) share: how a card names a component and one of its points. Each helper
// moves one kind of value (a component, a point of a component, a single component number); every card still lays out,
// reads, writes and checks its own fields by hand.
//
// An FRF component is an FRFCOMP card of the deck: the cards that use it (FRFCONN, FRFFLEX, FRFRELS, FRFSPC1, FBADLAY,
// FBALOAD, FBAPHAS) write "COMPID/COMPNAM" in one field, which holds the ID of the component or its name. The two are
// told apart by what the field is: a whole number is an ID, any other text is a name. The point that goes with the
// component is a point of the component's own model (a previous solver run made it), so it is a plain number in the
// deck being read: no GRID is looked up for it.
namespace oor::model
{

// The component that a card names: the ID of an FRFCOMP (a reference, renumbered with the FRF components) or its name
// (kept as written, never an ID, compared without regard to the case). A blank field has neither.
struct FrfComponentRef
{
    base::EntityId id = 0; // the ID of the FRFCOMP; 0 when the card names the component by its name
    std::string name;      // the name of the component; empty when the card names it by its ID

    [[nodiscard]] static FrfComponentRef byId(base::EntityId id) { return FrfComponentRef{id, {}}; }
    [[nodiscard]] static FrfComponentRef byName(std::string name) { return FrfComponentRef{0, std::move(name)}; }

    [[nodiscard]] bool isBlank() const noexcept { return id == 0 && name.empty(); }
    [[nodiscard]] bool operator==(const FrfComponentRef&) const noexcept = default;
};

// A component and one of its connection points (the COMPIDi/COMPNAMi and POINTi of an FRFCONN, FRFFLEX, FRFRELS and
// FRFSPC1). A group that a card may leave out is blank in both members.
struct FrfComponentPoint
{
    FrfComponentRef component;
    base::EntityId point = 0;

    [[nodiscard]] bool isBlank() const noexcept { return component.isBlank() && point == 0; }
    [[nodiscard]] bool operator==(const FrfComponentPoint&) const noexcept = default;
};

// The names of the fields of one group (names of the code, number included; a diagnostic shows their keywords). The
// field of the component holds its ID or its name.
struct FrfPointNames
{
    const char* component;
    const char* point;
};

// A text that reads back as a component ID: the lexer gives it to a card as a whole number.
[[nodiscard]] inline bool readsAsComponentId(std::string_view text) noexcept
{
    return base::NumberParser::parseInteger(text).has_value();
}

// The problem with the name of a component, if there is one: one to eight characters that read back from every format
// (no blank, comma, dollar sign or quote), and not a whole number, which every card that names the component would
// read as its ID.
[[nodiscard]] inline std::optional<CardProblem> checkFrfName(std::string_view name, const char* field)
{
    if (const auto problem = checkNameWord(name, field))
        return problem;
    if (readsAsComponentId(name))
    {
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is a whole number, which would be read as a component ID", field};
    }
    return std::nullopt;
}

// Reads the field that holds the ID or the name of a component. Blank gives a blank component, a whole number the ID
// (an error when it does not fit in 32 bits), any other text the name as written.
[[nodiscard]] inline FrfComponentRef readFrfComponent(FieldReader& reader, int field, const char* idName)
{
    FrfComponentRef component;
    const std::string_view text = reader.text(field);
    if (text.empty())
        return component;
    if (readsAsComponentId(text))
        component.id = reader.requiredInteger(field, idName);
    else
        component.name = std::string(text);
    return component;
}

// The problem with a component that the card requires, if there is one: exactly one of the ID (positive) and the name,
// in the field `field`.
[[nodiscard]] inline std::optional<CardProblem> checkFrfComponent(const FrfComponentRef& component, const char* field)
{
    const std::string label(CardProblem::FIELD);
    if (component.id != 0 && !component.name.empty())
        return CardProblem{"card.bad-value", label + " holds the ID or the name of an FRF component, not both", field};
    if (component.isBlank())
        return CardProblem{"card.missing-field", label + " is required: the ID or the name of an FRF component", field};
    if (component.id < 0)
        return CardProblem{"card.bad-id", label + " must be a positive integer", field};
    if (component.id == 0)
        return checkFrfName(component.name, field);
    return std::nullopt;
}

// Writes the ID as an integer, or the name as text.
inline void writeFrfComponent(FieldWriter& writer, int field, const FrfComponentRef& component)
{
    if (component.id != 0)
        writer.integer(field, component.id);
    else
        writer.text(field, component.name);
}

// Visits the ID of a component (a name is not an ID and is left alone, as is a blank field).
inline void visitFrfComponent(ReferenceVisitor& visitor, std::string_view field, FrfComponentRef& component)
{
    if (component.id > 0)
        component.id = visitor.visit(field, ReferenceTarget::FrfComponent, component.id);
}

// Reads a group: the component in `firstField` and its point in the next one.
[[nodiscard]] inline FrfComponentPoint readFrfComponentPoint(FieldReader& reader, int firstField,
                                                             const FrfPointNames& names)
{
    FrfComponentPoint group;
    group.component = readFrfComponent(reader, firstField, names.component);
    group.point = reader.integer(firstField + 1, names.point, 0);
    return group;
}

// The problem with a group, if there is one. A group that the card may leave out (`required` false) is fine when it is
// blank; half of one is not.
[[nodiscard]] inline std::optional<CardProblem> checkFrfComponentPoint(const FrfComponentPoint& group,
                                                                       const FrfPointNames& names, bool required)
{
    if (!required && group.isBlank())
        return std::nullopt;
    if (const auto problem = checkFrfComponent(group.component, names.component))
        return problem;
    if (group.point == 0)
    {
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", names.point};
    }
    return checkRequiredId(group.point, names.point);
}

inline void writeFrfComponentPoint(FieldWriter& writer, int firstField, const FrfComponentPoint& group)
{
    writeFrfComponent(writer, firstField, group.component);
    writer.integer(firstField + 1, group.point);
}

// A single component number (the C of an FRFFLEX, an FRFXIT and the three FBA cards): 1 to 6 for a grid point, 0 or
// blank for a scalar point.
[[nodiscard]] inline std::optional<CardProblem> checkSingleComponent(std::int32_t component, const char* field)
{
    if (component < 0 || component > 6)
    {
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " must be a single component, 1 to 6 for a grid point, 0 or blank for a scalar point",
                           field};
    }
    return std::nullopt;
}

} // namespace oor::model
