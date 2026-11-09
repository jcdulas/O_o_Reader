#include "oor/model/cards/elements/elemGeneration/ElemGenerationTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_OPTION_FIELD = 5;
constexpr int LAST_FIELD = FIRST_OPTION_FIELD + static_cast<int>(ELEM_GENERATION_OPTION_COUNT) - 1;

// The names of the fields 5 to 12, for the messages (the card keeps them as text).
constexpr std::array<const char*, ELEM_GENERATION_OPTION_COUNT> OPTION_NAMES{
    "direction", "thicknessOption", "lowElemId", "highElemId", "valueA", "valueB", "valueC", "valueD"};

} // namespace

std::optional<ElemGenerationCard> ElemGenerationFormat::read(FieldReader& reader)
{
    ElemGenerationCard card;
    if (reader.isBlank(1))
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
    card.kind = std::string(reader.text(1));
    card.firstElemId = reader.requiredInteger(2, "firstElemId");
    card.propId = reader.requiredInteger(3, "propId");
    card.gridFieldId = reader.requiredInteger(4, "gridFieldId");
    for (std::size_t index = 0; index < ELEM_GENERATION_OPTION_COUNT; ++index)
        card.options[index] = std::string(reader.text(FIRST_OPTION_FIELD + static_cast<int>(index)));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has twelve fields, " + std::string("kind") + " to " +
                         std::string("valueD"),
                     OPTION_NAMES.back());
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ElemGenerationFormat::check(const ElemGenerationCard& card)
{
    if (const auto problem = checkNameWord(card.kind, "kind"))
        return problem;
    if (const auto problem = checkRequiredId(card.firstElemId, "firstElemId"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkRequiredId(card.gridFieldId, "gridFieldId"))
        return problem;
    // A field that is not blank must read back from every format, as a word does.
    for (std::size_t index = 0; index < ELEM_GENERATION_OPTION_COUNT; ++index)
    {
        if (!card.options[index].empty())
        {
            if (const auto problem = checkNameWord(card.options[index], OPTION_NAMES[index]))
                return problem;
        }
    }
    return std::nullopt;
}

void ElemGenerationFormat::write(const ElemGenerationCard& card, FieldWriter& writer)
{
    writer.text(1, card.kind);
    writer.integer(2, card.firstElemId);
    writer.integer(3, card.propId);
    writer.integer(4, card.gridFieldId);
    for (std::size_t index = 0; index < ELEM_GENERATION_OPTION_COUNT; ++index)
    {
        if (!card.options[index].empty())
            writer.text(FIRST_OPTION_FIELD + static_cast<int>(index), card.options[index]);
    }
}

std::uint32_t ElemGenerationTable::append(const ElemGenerationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_kind.push(card.kind);
    m_firstElemId.push(card.firstElemId);
    m_propId.push(card.propId);
    m_gridFieldId.push(card.gridFieldId);
    m_cards.push(card);
    return row;
}

void ElemGenerationTable::store(std::uint32_t row, const ElemGenerationCard& card)
{
    m_kind.set(row, card.kind);
    m_firstElemId.set(row, card.firstElemId);
    m_propId.set(row, card.propId);
    m_gridFieldId.set(row, card.gridFieldId);
    m_cards.set(row, card);
}

void ElemGenerationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_kind.keepRows(rows);
    m_firstElemId.keepRows(rows);
    m_propId.keepRows(rows);
    m_gridFieldId.keepRows(rows);
    m_cards.keepRows(rows);
}

void ElemGenerationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The property of the generated elements. FIELD_ID names a GRIDG, which no card types: a plain ID, not visited.
    ElemGenerationCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    m_propId.set(row, card.propId);
}

} // namespace oor::model
