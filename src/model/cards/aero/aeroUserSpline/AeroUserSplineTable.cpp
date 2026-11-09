#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 11;

// The problem with a component name that may be blank: an empty name is blank, any other must be a valid name.
std::optional<CardProblem> checkOptionalName(const std::string& name, const char* field)
{
    if (name.empty())
        return std::nullopt;
    return checkNameWord(name, field);
}

} // namespace

std::optional<AeroUserSplineCard> AeroUserSplineFormat::read(FieldReader& reader)
{
    AeroUserSplineCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupName = std::string(reader.text(2));
    if (card.groupName.empty())
        reader.report(CardProblem{"card.missing-field", "{field} is required", "groupName"});
    card.dependentNodeComponent = std::string(reader.text(3));
    card.independentNodeComponent = std::string(reader.text(4));
    card.dependentElemComponent = std::string(reader.text(5));
    card.independentElemComponent = std::string(reader.text(6));
    card.usage = readKeyword<SplineUsage>(reader, 8, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.aeroBoxListId = reader.integer(9, "aeroBoxListId", 0);
    card.factorListId = reader.integer(10, "factorListId", 0);
    card.labelListId = reader.integer(11, "labelListId", 0);
    requireBlankFields(reader, {7}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroUserSplineFormat::check(const AeroUserSplineCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem = checkOptionalName(card.dependentNodeComponent, "dependentNodeComponent"))
        return problem;
    if (const auto problem = checkOptionalName(card.independentNodeComponent, "independentNodeComponent"))
        return problem;
    if (const auto problem = checkOptionalName(card.dependentElemComponent, "dependentElemComponent"))
        return problem;
    if (const auto problem = checkOptionalName(card.independentElemComponent, "independentElemComponent"))
        return problem;
    // Each mesh needs its points or its elements (a structural mesh may give the elements alone).
    if (card.dependentNodeComponent.empty() && card.dependentElemComponent.empty())
        return CardProblem{"card.missing-field", "{field} and {field:dependentElemComponent} cannot both be blank",
                           "dependentNodeComponent"};
    if (card.independentNodeComponent.empty() && card.independentElemComponent.empty())
        return CardProblem{"card.missing-field", "{field} and {field:independentElemComponent} cannot both be blank",
                           "independentNodeComponent"};
    if (const auto problem = checkOptionalId(card.aeroBoxListId, "aeroBoxListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.factorListId, "factorListId"))
        return problem;
    return checkOptionalId(card.labelListId, "labelListId");
}

void AeroUserSplineFormat::write(const AeroUserSplineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    if (!card.dependentNodeComponent.empty())
        writer.text(3, card.dependentNodeComponent);
    if (!card.independentNodeComponent.empty())
        writer.text(4, card.independentNodeComponent);
    if (!card.dependentElemComponent.empty())
        writer.text(5, card.dependentElemComponent);
    if (!card.independentElemComponent.empty())
        writer.text(6, card.independentElemComponent);
    writeKeywordUnlessDefault(writer, 8, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    writer.integerUnlessDefault(9, card.aeroBoxListId, 0);
    writer.integerUnlessDefault(10, card.factorListId, 0);
    writer.integerUnlessDefault(11, card.labelListId, 0);
}

std::uint32_t AeroUserSplineTable::append(const AeroUserSplineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroUserSplineTable::store(std::uint32_t row, const AeroUserSplineCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroUserSplineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroUserSplineTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroUserSplineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroUserSplineCard& card = m_cards.edit(row);
    visitId(visitor, "aeroBoxListId", ReferenceTarget::AeroList, card.aeroBoxListId);
    visitId(visitor, "factorListId", ReferenceTarget::AeroFactor, card.factorListId);
    // The component names are labels and the AELISTC a plain ID: nothing to follow in a renumbering.
}

} // namespace oor::model
