#include "oor/model/cards/aero/aeroComponent/AeroComponentTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 3;

} // namespace

std::optional<ReferenceTarget> aeroComponentTarget(AeroComponentListType type) noexcept
{
    switch (type)
    {
    case AeroComponentListType::AeroPanel:
        return ReferenceTarget::AeroElement;
    case AeroComponentListType::AeroBoxList:
        return ReferenceTarget::AeroList;
    case AeroComponentListType::NodeSet:
        return ReferenceTarget::Set;
    case AeroComponentListType::ComponentId:
        break;
    }
    return std::nullopt;
}

std::optional<AeroComponentCard> AeroComponentFormat::read(FieldReader& reader)
{
    AeroComponentCard card;
    card.name = std::string(reader.text(1));
    card.listKind = readRequiredKeyword<AeroComponentListType>(reader, 2, "listKind", AERO_COMPONENT_LIST_TYPE_WORDS);
    const int last = reader.lastUsedField();
    for (int field = FIRST_ID_FIELD; field <= last; ++field)
    {
        // No blank between two IDs, and no THRU (the integer reader refuses the word).
        if (reader.isBlank(field))
        {
            reader.report(CardProblem{"card.bad-value", "a blank field inside the list of {field}", "listIds"});
            continue;
        }
        card.ids.push_back(reader.requiredInteger(field, "listIds"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroComponentFormat::check(const AeroComponentCard& card)
{
    if (const auto problem = checkNameWord(card.name, "name"))
        return problem;
    if (card.ids.empty())
        return CardProblem{"card.missing-field", "{field} is required", "listIds"};
    for (const base::EntityId id : card.ids)
    {
        if (const auto problem = checkRequiredId(id, "listIds"))
            return problem;
    }
    return std::nullopt;
}

void AeroComponentFormat::write(const AeroComponentCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writeKeyword(writer, 2, AERO_COMPONENT_LIST_TYPE_WORDS, card.listKind);
    int field = FIRST_ID_FIELD;
    for (const base::EntityId id : card.ids)
        writer.integer(field++, id);
}

std::uint32_t AeroComponentTable::append(const AeroComponentCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void AeroComponentTable::store(std::uint32_t row, const AeroComponentCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void AeroComponentTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroComponentTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroComponentCard& card = m_cards.edit(row);
    const std::optional<ReferenceTarget> target = aeroComponentTarget(card.listKind);
    if (!target)
        return;
    for (base::EntityId& id : card.ids)
        visitId(visitor, "listIds", *target, id);
}

} // namespace oor::model
