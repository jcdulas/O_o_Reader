#include "oor/model/cards/properties/barPropNonlinear/BarPropNonlinearTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // the C2 line is a continuation line

} // namespace

std::optional<BarPropNonlinearCard> BarPropNonlinearFormat::read(FieldReader& reader)
{
    BarPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.sectionIntegration =
        readKeyword(reader, 4, "sectionIntegration", SECTION_INTEGRATION_WORDS, SectionIntegration::Smeared);
    requireUnusedFields(reader, {3, 5, 6, 7, 8});
    readBehaviorLines(reader, FIRST_KEYWORD_FIELD, BAR_PROP_NONLINEAR_GROUPS, card.groups);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BarPropNonlinearFormat::check(const BarPropNonlinearCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    return checkBehaviorLines(BAR_PROP_NONLINEAR_GROUPS, card.groups);
}

void BarPropNonlinearFormat::write(const BarPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writeKeywordUnlessDefault(writer, 4, SECTION_INTEGRATION_WORDS, card.sectionIntegration,
                              SectionIntegration::Smeared);
    writeBehaviorLines(writer, FIRST_KEYWORD_FIELD, BAR_PROP_NONLINEAR_GROUPS, card.groups);
}

std::uint32_t BarPropNonlinearTable::append(const BarPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BarPropNonlinearTable::store(std::uint32_t row, const BarPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BarPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BarPropNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BarPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    BarPropNonlinearCard& card = m_cards.edit(row);
    // The ID is the one of the PBAR or PBARL that the card extends (the two share the property space).
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
