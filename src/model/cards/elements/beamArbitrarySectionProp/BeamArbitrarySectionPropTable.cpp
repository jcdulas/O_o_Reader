#include "oor/model/cards/elements/beamArbitrarySectionProp/BeamArbitrarySectionPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TEXT_FIELD = 9; // the description starts on the first continuation line

} // namespace

std::optional<BeamArbitrarySectionPropCard> BeamArbitrarySectionPropFormat::read(FieldReader& reader)
{
    BeamArbitrarySectionPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.form = readRequiredKeyword<ArbitrarySectionForm>(reader, 3, "form", ARBITRARY_SECTION_FORM_WORDS);
    requireUnusedFields(reader, {4, 5, 6, 7, 8});
    card.items = readSectionItems(reader, FIRST_TEXT_FIELD, BEAM_SECTION_KEYS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamArbitrarySectionPropFormat::check(const BeamArbitrarySectionPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    return checkSectionItems(card.items, card.form, BEAM_SECTION_KEYS);
}

void BeamArbitrarySectionPropFormat::write(const BeamArbitrarySectionPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writeKeyword(writer, 3, ARBITRARY_SECTION_FORM_WORDS, card.form);
    writeSectionItems(writer, FIRST_TEXT_FIELD, card.items);
}

std::uint32_t BeamArbitrarySectionPropTable::append(const BeamArbitrarySectionPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BeamArbitrarySectionPropTable::store(std::uint32_t row, const BeamArbitrarySectionPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BeamArbitrarySectionPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BeamArbitrarySectionPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BeamArbitrarySectionPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    BeamArbitrarySectionPropCard& card = m_cards.edit(row);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    // The SET1 or SET3 of OUTP, INP, BRP and LAYER, the PCOMP or PCOMPG of CORE and LAYER, the POINTs of PT=().
    visitSectionItems(card.items, visitor);
}

} // namespace oor::model
