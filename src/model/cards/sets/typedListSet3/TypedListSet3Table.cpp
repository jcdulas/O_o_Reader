#include "oor/model/cards/sets/typedListSet3/TypedListSet3Table.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 3;

struct NamedKind
{
    TypedListSet3Kind kind;
    std::string_view name;
};

// The words of the kinds: Node and GeomPoint are the names of card types, the others words of their own.
constexpr NamedKind KINDS[] = {
    {TypedListSet3Kind::Grid, "Node"},
    {TypedListSet3Kind::Element, "Elements"},
    {TypedListSet3Kind::Point, "GeomPoint"},
    {TypedListSet3Kind::Property, "Property"},
    {TypedListSet3Kind::RigidIncluded, "RigidElemsIncluded"},
    {TypedListSet3Kind::RigidExcluded, "RigidElemsExcluded"},
    {TypedListSet3Kind::Module, "Module"},
};

} // namespace

std::string_view TypedListSet3Format::kindName(TypedListSet3Kind kind)
{
    for (const NamedKind& entry : KINDS)
    {
        if (entry.kind == kind)
            return entry.name;
    }
    return {};
}

std::optional<TypedListSet3Kind> TypedListSet3Format::kindFromName(std::string_view name)
{
    for (const NamedKind& entry : KINDS)
    {
        if (name == entry.name)
            return entry.kind;
    }
    return std::nullopt;
}

std::optional<TypedListSet3Card> TypedListSet3Format::read(FieldReader& reader)
{
    TypedListSet3Card card;
    card.id = reader.requiredInteger(1, "id");
    if (const auto kind = kindFromName(reader.text(2)))
        card.kind = *kind;
    else
        reader.error(
            "card.bad-value",
            std::string("kind") +
                " must be Node, Elements, GeomPoint, Property, RigidElemsIncluded, RigidElemsExcluded or Module",
            "kind");
    card.ids = readIdRuns(reader, FIRST_ID_FIELD, "ids");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TypedListSet3Format::check(const TypedListSet3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    return checkIdRuns(card.ids, "ids", IdSign::NotNegative);
}

void TypedListSet3Format::write(const TypedListSet3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, kindName(card.kind));
    writeIdRuns(writer, FIRST_ID_FIELD, card.ids, writer.compressesRanges());
}

std::uint32_t TypedListSet3Table::append(const TypedListSet3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_runs.push(toPairs(card.ids));
    return row;
}

TypedListSet3Card TypedListSet3Table::card(std::uint32_t row) const
{
    TypedListSet3Card card;
    card.id = m_id.get(row);
    card.kind = static_cast<TypedListSet3Kind>(m_kind.get(row));
    card.ids = toIdRuns(m_runs.get(row));
    return card;
}

void TypedListSet3Table::store(std::uint32_t row, const TypedListSet3Card& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_runs.set(row, toPairs(card.ids));
}

void TypedListSet3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_runs.keepRows(rows);
}

void TypedListSet3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    switch (static_cast<TypedListSet3Kind>(m_kind.get(row)))
    {
    case TypedListSet3Kind::Grid:
        visitIdRuns(m_runs, row, visitor, "ids", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
        break;
    case TypedListSet3Kind::Element:
    case TypedListSet3Kind::RigidIncluded:
    case TypedListSet3Kind::RigidExcluded:
        visitIdRuns(m_runs, row, visitor, "ids", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
        break;
    case TypedListSet3Kind::Property:
        // There is no target for the end of a range of properties: they are not looked at.
        visitIdRuns(m_runs, row, visitor, "ids", ReferenceTarget::Property, ReferenceTarget::Other);
        break;
    case TypedListSet3Kind::Point:
    case TypedListSet3Kind::Module:
        break; // aero points and modules are not cards of the model
    }
}

} // namespace oor::model
