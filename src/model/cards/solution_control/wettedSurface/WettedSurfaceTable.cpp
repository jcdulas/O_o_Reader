#include "oor/model/cards/solution_control/wettedSurface/WettedSurfaceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/ExternalName.hpp"

namespace oor::model
{

namespace
{

constexpr int WETTED_TAG_FIELD = 2;
constexpr int FIRST_ELEMENT_FIELD = 9; // WEID1, the first field of the first continuation line

// THRU and BY are allowed, and a run goes upwards (the format draws no descending list).
constexpr ContactRunRules RUN_RULES{true, false};

} // namespace

std::optional<WettedSurfaceCard> WettedSurfaceFormat::read(FieldReader& reader)
{
    WettedSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.wettedTag = std::string(reader.text(WETTED_TAG_FIELD));
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, reader.lastUsedField());
    if (reader.failed())
        return card;
    card.runs = readContactRuns(reader, FIRST_ELEMENT_FIELD, reader.lastUsedField(), "wettedElemIds", RUN_RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WettedSurfaceFormat::check(const WettedSurfaceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkExternalName(card.wettedTag, "wettedTag"))
        return problem;
    if (card.runs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one wetted element (" + std::string(CardProblem::FIELD) + ")",
                           "wettedElemIds"};
    return checkContactRuns(card.runs, "wettedElemIds", RUN_RULES);
}

void WettedSurfaceFormat::write(const WettedSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(WETTED_TAG_FIELD, card.wettedTag);
    writeContactRuns(writer, FIRST_ELEMENT_FIELD, card.runs);
}

std::uint32_t WettedSurfaceTable::append(const WettedSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void WettedSurfaceTable::store(std::uint32_t row, const WettedSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void WettedSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void WettedSurfaceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void WettedSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The WSID is the card's own number, and the tag a name: only the wetted elements are references. The kind has no
    // "range" form, so the two ends of a run must exist like a single ID.
    visitContactRuns(m_cards.edit(row).runs, visitor, "wettedElemIds", ReferenceTarget::WettedElement,
                     ReferenceTarget::WettedElement);
}

} // namespace oor::model
