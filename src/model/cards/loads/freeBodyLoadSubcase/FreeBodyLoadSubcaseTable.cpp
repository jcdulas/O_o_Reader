#include "oor/model/cards/loads/freeBodyLoadSubcase/FreeBodyLoadSubcaseTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/FreeBodySupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/core/Model.hpp"

#include <set>

namespace oor::model
{

std::optional<FreeBodyLoadSubcaseCard> FreeBodyLoadSubcaseFormat::read(FieldReader& reader)
{
    FreeBodyLoadSubcaseCard card;
    card.names = std::string(reader.text(1));
    card.nodeSetId = reader.requiredInteger(2, "nodeSetId");
    card.elementSetId = reader.integer(3, "elementSetId", 0);
    card.excludedForces = base::toUpperAscii(reader.text(4));
    // Fields 5 to 8 are blank in the layout of the format; the label is the optional second line.
    requireBlankFields(reader, {5, 6, 7, 8}, FREE_BODY_LABEL_LAST_FIELD);
    card.label = readFreeBodyLabel(reader);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeBodyLoadSubcaseFormat::check(const FreeBodyLoadSubcaseCard& card)
{
    if (const auto problem = checkFreeBodyName(card.names, "names"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.elementSetId, "elementSetId"))
        return problem;
    for (const char letter : card.excludedForces)
    {
        if (FREE_BODY_EXCLUSION_LETTERS.find(letter) == std::string_view::npos)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is a combination of the letters S, M, A, L, P and D",
                               "excludedForces"};
    }
    return checkFreeBodyLabel(card.label);
}

void FreeBodyLoadSubcaseFormat::write(const FreeBodyLoadSubcaseCard& card, FieldWriter& writer)
{
    writer.text(1, card.names);
    writer.integer(2, card.nodeSetId);
    writer.integerUnlessDefault(3, card.elementSetId, 0);
    writer.text(4, card.excludedForces);
    writeFreeBodyLabel(writer, card.label);
}

std::uint32_t FreeBodyLoadSubcaseTable::append(const FreeBodyLoadSubcaseCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_names.push(card.names);
    m_nodeSetId.push(card.nodeSetId);
    m_elementSetId.push(card.elementSetId);
    m_excludedForces.push(card.excludedForces);
    m_label.push(card.label);
    return row;
}

FreeBodyLoadSubcaseCard FreeBodyLoadSubcaseTable::card(std::uint32_t row) const
{
    FreeBodyLoadSubcaseCard card;
    card.names = m_names.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.elementSetId = m_elementSetId.get(row);
    card.excludedForces = m_excludedForces.get(row);
    card.label = m_label.get(row);
    return card;
}

void FreeBodyLoadSubcaseTable::store(std::uint32_t row, const FreeBodyLoadSubcaseCard& card)
{
    m_names.set(row, card.names);
    m_nodeSetId.set(row, card.nodeSetId);
    m_elementSetId.set(row, card.elementSetId);
    m_excludedForces.set(row, card.excludedForces);
    m_label.set(row, card.label);
}

void FreeBodyLoadSubcaseTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_names.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_elementSetId.keepRows(rows);
    m_excludedForces.keepRows(rows);
    m_label.keepRows(rows);
}

void FreeBodyLoadSubcaseTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // Both name a set of the Bulk Data (the format says a SET1); a blank ELEMSET names nothing.
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
    if (const base::EntityId elementSetId = m_elementSetId.get(row); elementSetId > 0)
        m_elementSetId.set(row, visitor.visit("elementSetId", ReferenceTarget::Set, elementSetId));
}

void FreeBodyLoadSubcaseTable::finishRead(Model&, const ReadContext& context)
{
    // The name of a subsystem is compared without regard to the case, like every name a card gives another card. The
    // format reference does not say that it is unique, but an FBODYLD names a subsystem by it: a repeat is a warning.
    std::set<std::string> names;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        const std::string name = m_names.get(row);
        const std::string key = std::string(name);
        if (!names.insert(key).second && reported.insert(key).second)
            context.sink.warning("deck.duplicate-free-body-load-subcase",
                                 "the " + std::string(NAME) + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
