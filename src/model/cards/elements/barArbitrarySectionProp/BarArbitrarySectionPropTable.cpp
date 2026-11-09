#include "oor/model/cards/elements/barArbitrarySectionProp/BarArbitrarySectionPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TEXT_FIELD = 9; // the description starts on the first continuation line

} // namespace

std::optional<BarArbitrarySectionPropCard> BarArbitrarySectionPropFormat::read(FieldReader& reader)
{
    BarArbitrarySectionPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.form = readRequiredKeyword<ArbitrarySectionForm>(reader, 3, "form", ARBITRARY_SECTION_FORM_WORDS);
    card.nonStructMass = reader.real(4, "nonStructMass", 0.0);
    requireUnusedFields(reader, {5, 6, 7, 8});
    card.items = readSectionItems(reader, FIRST_TEXT_FIELD, BAR_SECTION_KEYS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BarArbitrarySectionPropFormat::check(const BarArbitrarySectionPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.nonStructMass, "nonStructMass", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkSectionItems(card.items, card.form, BAR_SECTION_KEYS))
        return problem;
    // The mass is given in the field or in the text, not in both (the format reference lists it in both places).
    if (card.nonStructMass != 0.0)
    {
        for (const ArbitrarySectionItem& item : card.items)
        {
            if (item.key == ArbitrarySectionKey::NonstructuralMass)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " is given in its field and in the description",
                                   "nonStructMass"};
        }
    }
    return std::nullopt;
}

void BarArbitrarySectionPropFormat::write(const BarArbitrarySectionPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writeKeyword(writer, 3, ARBITRARY_SECTION_FORM_WORDS, card.form);
    writer.realUnlessDefault(4, card.nonStructMass, 0.0);
    writeSectionItems(writer, FIRST_TEXT_FIELD, card.items);
}

std::uint32_t BarArbitrarySectionPropTable::append(const BarArbitrarySectionPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BarArbitrarySectionPropTable::store(std::uint32_t row, const BarArbitrarySectionPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BarArbitrarySectionPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BarArbitrarySectionPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void BarArbitrarySectionPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    BarArbitrarySectionPropCard& card = m_cards.edit(row);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    // The SET1 or SET3 of OUTP, INP and BRP, and the POINTs of PT=().
    visitSectionItems(card.items, visitor);
}

} // namespace oor::model
