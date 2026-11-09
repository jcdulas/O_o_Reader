#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<ContactPairCard> ContactPairFormat::read(FieldReader& reader)
{
    ContactPairCard card;
    card.id = reader.requiredInteger(1, "id");
    card.geomParamsId = reader.integer(2, "geomParamsId", 0);
    card.physicalParamsId = reader.integer(3, "physicalParamsId", 0);
    card.secondaryBodyId = reader.integer(4, "secondaryBodyId", 0);
    card.primaryBodyId = reader.integer(5, "primaryBodyId", 0);
    requireUnusedBlank(reader, {6, 7, 8}, reader.lastUsedField());
    const std::vector<ContactSection> sections = splitContactSections(reader, {"Secondary", "Primary"});
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty())
        {
            // The head has the first line only: a continuation line with no keyword is not a BCONECT line.
            if (section.last > CONTACT_LINE_FIELDS)
                reader.error(
                    "card.bad-value",
                    "a continuation line of a " + std::string(reader.cardName()) + " starts with SECNDRY or PRIMARY",
                    "secondaryList");
            continue;
        }
        std::vector<base::EntityId> ids = readContactIds(reader, section.first + 1, section.last, "secondaryBodies");
        std::vector<base::EntityId>& list = section.keyword == "Secondary" ? card.secondaryBodies : card.primaryBodies;
        list.insert(list.end(), ids.begin(), ids.end());
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactPairFormat::check(const ContactPairCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    if (card.geomParamsId < 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be the ID of the geometric parameters of a pair, or blank",
            "geomParamsId"};
    if (card.physicalParamsId < 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be the ID of the physical parameters of a pair, or blank",
            "physicalParamsId"};
    if (card.secondaryBodyId < 0 || card.primaryBodyId < 0)
        return CardProblem{"card.bad-id", "a body ID must not be negative",
                           card.secondaryBodyId < 0 ? "secondaryBodyId" : "primaryBodyId"};
    for (const base::EntityId body : card.secondaryBodies)
    {
        if (body < 0)
            return CardProblem{"card.bad-id", "a body ID must not be negative", "secondaryBodies"};
    }
    for (const base::EntityId body : card.primaryBodies)
    {
        if (body < 0)
            return CardProblem{"card.bad-id", "a body ID must not be negative", "primaryBodyId"};
    }
    // The touching body is the short field or the first of the SECNDRY list; in SOL 700 a self-contact has the
    // secondary list 0 and no primary body at all.
    if (card.secondaryBodyId == 0 && card.secondaryBodies.empty())
        return CardProblem{"card.missing-field",
                           "the pair needs " + std::string(CardProblem::FIELD) + " or a SECNDRY list",
                           "secondaryBodyId"};
    const bool selfContact = card.secondaryBodies.size() == 1 && card.secondaryBodies.front() == 0;
    if (card.primaryBodyId == 0 && card.primaryBodies.empty() && !selfContact)
        return CardProblem{"card.missing-field",
                           "the pair needs " + std::string(CardProblem::FIELD) + " or a PRIMARY list", "primaryBodyId"};
    return std::nullopt;
}

void ContactPairFormat::write(const ContactPairCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.geomParamsId, 0);
    writer.integerUnlessDefault(3, card.physicalParamsId, 0);
    writer.integerUnlessDefault(4, card.secondaryBodyId, 0);
    writer.integerUnlessDefault(5, card.primaryBodyId, 0);
    int line = firstFieldOfLine(1);
    if (!card.secondaryBodies.empty())
        line = writeContactIdList(writer, line, "Secondary", card.secondaryBodies);
    if (!card.primaryBodies.empty())
        writeContactIdList(writer, line, "Primary", card.primaryBodies);
}

std::uint32_t ContactPairTable::append(const ContactPairCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_geomParamsId.push(card.geomParamsId);
    m_physicalParamsId.push(card.physicalParamsId);
    m_secondaryBodyId.push(card.secondaryBodyId);
    m_primaryBodyId.push(card.primaryBodyId);
    m_secondaryBodies.push(card.secondaryBodies);
    m_primaryBodies.push(card.primaryBodies);
    return row;
}

ContactPairCard ContactPairTable::card(std::uint32_t row) const
{
    ContactPairCard card;
    card.id = m_id.get(row);
    card.geomParamsId = m_geomParamsId.get(row);
    card.physicalParamsId = m_physicalParamsId.get(row);
    card.secondaryBodyId = m_secondaryBodyId.get(row);
    card.primaryBodyId = m_primaryBodyId.get(row);
    const auto secondaryBodies = m_secondaryBodies.get(row);
    card.secondaryBodies.assign(secondaryBodies.begin(), secondaryBodies.end());
    const auto primaryBodies = m_primaryBodies.get(row);
    card.primaryBodies.assign(primaryBodies.begin(), primaryBodies.end());
    return card;
}

void ContactPairTable::store(std::uint32_t row, const ContactPairCard& card)
{
    m_id.set(row, card.id);
    m_geomParamsId.set(row, card.geomParamsId);
    m_physicalParamsId.set(row, card.physicalParamsId);
    m_secondaryBodyId.set(row, card.secondaryBodyId);
    m_primaryBodyId.set(row, card.primaryBodyId);
    m_secondaryBodies.set(row, card.secondaryBodies);
    m_primaryBodies.set(row, card.primaryBodies);
}

void ContactPairTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_geomParamsId.keepRows(rows);
    m_physicalParamsId.keepRows(rows);
    m_secondaryBodyId.keepRows(rows);
    m_primaryBodyId.keepRows(rows);
    m_secondaryBodies.keepRows(rows);
    m_primaryBodies.keepRows(rows);
}

void ContactPairTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdAt(m_geomParamsId, row, visitor, "geomParamsId", ReferenceTarget::ContactGeometricParameters);
    visitContactIdAt(m_physicalParamsId, row, visitor, "physicalParamsId", ReferenceTarget::ContactPhysicalParameters);
    visitContactIdAt(m_secondaryBodyId, row, visitor, "secondaryBodyId", ReferenceTarget::ContactBody);
    visitContactIdAt(m_primaryBodyId, row, visitor, "primaryBodyId", ReferenceTarget::ContactBody);
    visitContactIdColumn(m_secondaryBodies, row, visitor, "secondaryBodies", ReferenceTarget::ContactBody);
    visitContactIdColumn(m_primaryBodies, row, visitor, "primaryBodyId", ReferenceTarget::ContactBody);
}

} // namespace oor::model
