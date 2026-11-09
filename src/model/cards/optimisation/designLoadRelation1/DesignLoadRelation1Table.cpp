#include "oor/model/cards/optimisation/designLoadRelation1/DesignLoadRelation1Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/design/DesignFieldNames.hpp"

#include <algorithm>
#include <cctype>

namespace oor::model
{

namespace
{

constexpr int FIRST_ATTRIBUTE_FIELD = 9;
constexpr int FIRST_TERM_FIELD = 17;

// Each supported load uses its neutral scalar or indexed term name.
bool isNameOfLoadValue(const std::string& type, const std::string& name)
{
    if (type == "PointForceLoad" || type == "PointMomentLoad")
        return name == "magnitude" || name == "directionX" || name == "directionY" || name == "directionZ";
    if (type == "LoadCombination")
    {
        const auto indexed = splitTrailingNumber(name);
        return name == "scaleFactor" || (indexed.number > 0 && indexed.word == "termScaleFactor");
    }
    return !checkDesignFieldName(name, "fieldName");
}

} // namespace

LoadAttribute loadAttributeOf(const DesignLoadRelation1Card& card, std::size_t index)
{
    if (card.kind == "PointForceLoad" || card.kind == "PointMomentLoad")
    {
        if (index == 0)
            return LoadAttribute::Grid;
        if (index == 1)
            return LoadAttribute::Coordinate;
    }
    else if (card.kind == "LoadCombination" && index == 0 && card.fieldName != "scaleFactor")
    {
        return LoadAttribute::LoadCombinationSet;
    }
    return LoadAttribute::Unused;
}

std::optional<DesignLoadRelation1Card> DesignLoadRelation1Format::read(FieldReader& reader)
{
    DesignLoadRelation1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = std::string(reader.text(2));
    if (card.kind.empty())
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
    card.setId = reader.requiredInteger(3, "setId");
    card.fieldName = std::string(reader.text(4));
    if (card.fieldName.empty())
        reader.error("card.missing-field", std::string("fieldName") + " is required", "fieldName");
    card.minValue = reader.real(5, "minValue", DesignLoadRelation1Card::NO_LOWER_LIMIT);
    card.maxValue = reader.real(6, "maxValue", DesignLoadRelation1Card::NO_UPPER_LIMIT);
    card.constant0 = reader.real(7, "constant0", 0.0);
    for (std::size_t index = 0; index < DesignLoadRelation1Card::ATTRIBUTE_COUNT; ++index)
        card.attributes[index] = reader.optionalInteger(FIRST_ATTRIBUTE_FIELD + static_cast<int>(index), "attributes");
    // The cells the format reference leaves blank: the end of the first line and the end of the attribute line.
    for (const int field : {8, 14, 15, 16})
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            break;
        }
    }
    card.terms = readDesignTerms(reader, FIRST_TERM_FIELD, "designVarIds", "coefficients", true, card.fromProperty);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignLoadRelation1Format::check(const DesignLoadRelation1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNeutralName(card.kind, "kind"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkDesignFieldName(card.fieldName, "fieldName"))
        return problem;
    if (!isNameOfLoadValue(card.kind, card.fieldName))
        return CardProblem{"card.bad-value", card.fieldName + " is not a designable value of " + card.kind,
                           "fieldName"};
    if (const auto problem = checkReal(card.minValue, "minValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.maxValue, "maxValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.constant0, "constant0", RealRule::Finite))
        return problem;
    for (const std::optional<std::int32_t>& attribute : card.attributes)
    {
        if (attribute && *attribute < 0)
            return CardProblem{"card.bad-value",
                               "an attribute " + std::string(CardProblem::FIELD) + " must not be negative",
                               "attributes"};
    }
    return checkDesignTerms(card.terms, card.fromProperty, false, "designVarIds", "coefficients");
}

void DesignLoadRelation1Format::write(const DesignLoadRelation1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.kind);
    writer.integer(3, card.setId);
    writer.text(4, card.fieldName);
    writer.realUnlessDefault(5, card.minValue, DesignLoadRelation1Card::NO_LOWER_LIMIT);
    writer.realUnlessDefault(6, card.maxValue, DesignLoadRelation1Card::NO_UPPER_LIMIT);
    writer.realUnlessDefault(7, card.constant0, 0.0);
    for (std::size_t index = 0; index < DesignLoadRelation1Card::ATTRIBUTE_COUNT; ++index)
    {
        if (card.attributes[index])
            writer.integer(FIRST_ATTRIBUTE_FIELD + static_cast<int>(index), *card.attributes[index]);
    }
    writeDesignTerms(writer, FIRST_TERM_FIELD, card.terms, card.fromProperty);
}

std::uint32_t DesignLoadRelation1Table::append(const DesignLoadRelation1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void DesignLoadRelation1Table::store(std::uint32_t row, const DesignLoadRelation1Card& card)
{
    m_id.set(row, card.id);
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void DesignLoadRelation1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignLoadRelation1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignLoadRelation1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignLoadRelation1Card& card = m_cards.edit(row);
    visitId(visitor, "setId", ReferenceTarget::LoadCombinationSet, card.setId);
    m_setId.set(row, card.setId);
    for (std::size_t index = 0; index < DesignLoadRelation1Card::ATTRIBUTE_COUNT; ++index)
    {
        std::optional<std::int32_t>& attribute = card.attributes[index];
        if (!attribute || *attribute <= 0)
            continue; // blank, or the 0 that a LOAD whose LNAME is S asks for
        switch (loadAttributeOf(card, index))
        {
        case LoadAttribute::Grid:
            attribute = visitor.visit("attributes", ReferenceTarget::Node, *attribute);
            break;
        case LoadAttribute::Coordinate:
            attribute = visitor.visit("attributes", ReferenceTarget::Coordinate, *attribute);
            break;
        case LoadAttribute::LoadCombinationSet:
            attribute = visitor.visit("attributes", ReferenceTarget::LoadCombinationSet, *attribute);
            break;
        case LoadAttribute::Unused:
            break;
        }
    }
    visitDesignTerms(visitor, card.terms, "designVarIds");
}

} // namespace oor::model
