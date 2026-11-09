#include "oor/model/cards/nonlinear_contact/contactBodyChange/ContactBodyChangeTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int RANGE_FIELDS = 4;
constexpr int FIRST_RANGE_FIELD = 3;

} // namespace

std::optional<ContactBodyChangeCard> ContactBodyChangeFormat::read(FieldReader& reader)
{
    ContactBodyChangeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = readKeyword(reader, 2, "kind", {"Node", "Exclude"}, "");
    if (reader.isBlank(2))
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " needs its " + std::string("kind"),
                     "kind");
    const int last = reader.lastUsedField();
    for (int field = FIRST_RANGE_FIELD; field <= last && !reader.failed(); field += RANGE_FIELDS)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2) &&
            reader.isBlank(field + 3))
            continue;
        ContactBodyChangeRange range;
        range.bodyId = reader.requiredInteger(field, "bodyId");
        range.firstNode = reader.integer(field + 1, "firstNode", 1);
        range.lastNode = reader.requiredInteger(field + 2, "lastNode");
        range.increment = reader.integer(field + 3, "increment", 0);
        card.ranges.push_back(range);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBodyChangeFormat::check(const ContactBodyChangeCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    if (auto problem = checkKeyword(card.kind, "kind", {"Node", "Exclude"}))
        return problem;
    if (card.ranges.empty())
        return CardProblem{"card.missing-field", "the card needs at least one body", "bodyId"};
    const bool exclude = card.kind == "Exclude";
    for (const ContactBodyChangeRange& range : card.ranges)
    {
        if (range.bodyId <= 0)
            return CardProblem{"card.bad-id", "a body ID must be a positive integer", "bodyId"};
        // NODE has N1 > 0 and N2 > 0; EXCLUDE may begin a patch with -N1.
        if (range.firstNode == 0 || (range.firstNode < 0 && !exclude))
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                               "firstNode"};
        if (range.lastNode <= 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                               "lastNode"};
        if (range.increment < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "increment"};
        if (!exclude && range.increment > 0 && range.firstNode >= range.lastNode)
            return CardProblem{"card.bad-value",
                               "a range with an increment needs " + CardProblem::fieldNamed("firstNode") + " below " +
                                   CardProblem::fieldNamed("lastNode"),
                               "lastNode"};
    }
    return std::nullopt;
}

void ContactBodyChangeFormat::write(const ContactBodyChangeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.kind);
    int field = FIRST_RANGE_FIELD;
    for (const ContactBodyChangeRange& range : card.ranges)
    {
        writer.integer(field, range.bodyId);
        writer.integerUnlessDefault(field + 1, range.firstNode, 1);
        writer.integer(field + 2, range.lastNode);
        writer.integerUnlessDefault(field + 3, range.increment, 0);
        field += RANGE_FIELDS;
    }
}

std::uint32_t ContactBodyChangeTable::append(const ContactBodyChangeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(card.kind);
    std::vector<std::array<base::EntityId, 4>> ranges;
    for (const ContactBodyChangeRange& range : card.ranges)
        ranges.push_back({range.bodyId, range.firstNode, range.lastNode, range.increment});
    m_ranges.push(ranges);
    return row;
}

ContactBodyChangeCard ContactBodyChangeTable::card(std::uint32_t row) const
{
    ContactBodyChangeCard card;
    card.id = m_id.get(row);
    card.kind = m_kind.get(row);
    for (const auto& entry : m_ranges.get(row))
        card.ranges.push_back(ContactBodyChangeRange{entry[0], entry[1], entry[2], entry[3]});
    return card;
}

void ContactBodyChangeTable::store(std::uint32_t row, const ContactBodyChangeCard& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, card.kind);
    std::vector<std::array<base::EntityId, 4>> ranges;
    for (const ContactBodyChangeRange& range : card.ranges)
        ranges.push_back({range.bodyId, range.firstNode, range.lastNode, range.increment});
    m_ranges.set(row, ranges);
}

void ContactBodyChangeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_ranges.keepRows(rows);
}

void ContactBodyChangeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto current = m_ranges.get(row);
    std::vector<std::array<base::EntityId, 4>> ranges(current.begin(), current.end());
    bool changed = false;
    // A minus sign on N1 marks the first range of a patch: the grid point is its absolute value.
    const auto visitGrid = [&](base::EntityId& id)
    {
        const base::EntityId magnitude = id < 0 ? -id : id;
        const base::EntityId updated = visitor.visit("firstNode", ReferenceTarget::NodeInRange, magnitude);
        const base::EntityId signedUpdated = id < 0 ? -updated : updated;
        changed = changed || signedUpdated != id;
        id = signedUpdated;
    };
    for (auto& entry : ranges)
    {
        const base::EntityId updated = visitor.visit("bodyId", ReferenceTarget::ContactBody, entry[0]);
        changed = changed || updated != entry[0];
        entry[0] = updated;
        visitGrid(entry[1]);
        visitGrid(entry[2]);
    }
    if (changed)
        m_ranges.set(row, ranges);
}

} // namespace oor::model
