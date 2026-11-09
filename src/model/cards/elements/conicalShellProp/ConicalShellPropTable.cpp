#include "oor/model/cards/elements/conicalShellProp/ConicalShellPropTable.hpp"

namespace oor::model
{

std::optional<ConicalShellPropCard> ConicalShellPropFormat::read(FieldReader& reader)
{
    ConicalShellPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId1 = reader.integer(2, "matId1", 0);
    card.thickness1 = reader.real(3, "thickness1", 0.0);
    card.matId2 = reader.integer(4, "matId2", 0);
    card.inertia = reader.real(5, "inertia", 0.0);
    card.matId3 = reader.integer(6, "matId3", 0);
    card.thickness2 = reader.real(7, "thickness2", 0.0);
    card.nonStructMass = reader.real(8, "nonStructMass", 0.0);
    card.fiberDistance1 = reader.real(9, "fiberDistance1", ConicalShellPropCard::NO_VALUE);
    card.fiberDistance2 = reader.real(10, "fiberDistance2", ConicalShellPropCard::NO_VALUE);
    // PHI1 to PHI14 are the fields 11 to 24; a blank one between two is skipped.
    for (int field = 11; field <= 24; ++field)
    {
        if (const auto value = reader.optionalReal(field, "phiAngle"))
            card.phiAngle.push_back(*value);
    }
    if (reader.lastUsedField() > 24)
        reader.error(
            "card.bad-value",
            "a " + std::string(reader.cardName()) + " has at most 14 azimuthal coordinates " + std::string("phiAngle"),
            "phiAngle");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ConicalShellPropFormat::check(const ConicalShellPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId1 < 0 || card.matId2 < 0 || card.matId3 < 0)
        return CardProblem{"card.bad-id", "the material IDs must not be negative", "matId1"};
    // The format reference: a material and its thickness (or inertia) are both given, or both zero or blank.
    if ((card.matId1 == 0) != (card.thickness1 == 0.0))
        return CardProblem{"card.bad-value", "{field:matId1} and {field} must both be given or both be zero or blank",
                           "thickness1"};
    if ((card.matId2 == 0) != (card.inertia == 0.0))
        return CardProblem{"card.bad-value", "{field:matId2} and {field} must both be given or both be zero or blank",
                           "inertia"};
    if ((card.matId3 == 0) != (card.thickness2 == 0.0))
        return CardProblem{"card.bad-value", "{field:matId3} and {field} must both be given or both be zero or blank",
                           "thickness2"};
    if (card.thickness1 < 0.0 || card.thickness2 < 0.0)
        return CardProblem{"card.bad-value", "the thicknesses {field} and {field:thickness2} must not be negative",
                           "thickness1"};
    if (card.phiAngle.size() > ConicalShellPropCard::MAX_PHI)
        return CardProblem{"card.bad-value", "the card has at most 14 azimuthal coordinates {field}", "phiAngle"};
    return std::nullopt;
}

void ConicalShellPropFormat::write(const ConicalShellPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId1, 0);
    writer.realUnlessDefault(3, card.thickness1, 0.0);
    writer.integerUnlessDefault(4, card.matId2, 0);
    writer.realUnlessDefault(5, card.inertia, 0.0);
    writer.integerUnlessDefault(6, card.matId3, 0);
    writer.realUnlessDefault(7, card.thickness2, 0.0);
    writer.realUnlessDefault(8, card.nonStructMass, 0.0);
    if (!std::isnan(card.fiberDistance1))
        writer.real(9, card.fiberDistance1);
    if (!std::isnan(card.fiberDistance2))
        writer.real(10, card.fiberDistance2);
    for (std::size_t index = 0; index < card.phiAngle.size(); ++index)
        writer.real(11 + static_cast<int>(index), card.phiAngle[index]);
}

std::uint32_t ConicalShellPropTable::append(const ConicalShellPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId1.push(card.matId1);
    m_thickness1.push(card.thickness1);
    m_matId2.push(card.matId2);
    m_inertia.push(card.inertia);
    m_matId3.push(card.matId3);
    m_thickness2.push(card.thickness2);
    m_nonStructMass.push(card.nonStructMass);
    m_fiberDistance1.push(card.fiberDistance1);
    m_fiberDistance2.push(card.fiberDistance2);
    m_phiAngle.push(card.phiAngle);
    return row;
}

ConicalShellPropCard ConicalShellPropTable::card(std::uint32_t row) const
{
    ConicalShellPropCard card;
    card.id = m_id.get(row);
    card.matId1 = m_matId1.get(row);
    card.thickness1 = m_thickness1.get(row);
    card.matId2 = m_matId2.get(row);
    card.inertia = m_inertia.get(row);
    card.matId3 = m_matId3.get(row);
    card.thickness2 = m_thickness2.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.fiberDistance1 = m_fiberDistance1.get(row);
    card.fiberDistance2 = m_fiberDistance2.get(row);
    const auto phi = m_phiAngle.get(row);
    card.phiAngle.assign(phi.begin(), phi.end());
    return card;
}

void ConicalShellPropTable::store(std::uint32_t row, const ConicalShellPropCard& card)
{
    m_id.set(row, card.id);
    m_matId1.set(row, card.matId1);
    m_thickness1.set(row, card.thickness1);
    m_matId2.set(row, card.matId2);
    m_inertia.set(row, card.inertia);
    m_matId3.set(row, card.matId3);
    m_thickness2.set(row, card.thickness2);
    m_nonStructMass.set(row, card.nonStructMass);
    m_fiberDistance1.set(row, card.fiberDistance1);
    m_fiberDistance2.set(row, card.fiberDistance2);
    m_phiAngle.set(row, card.phiAngle);
}

void ConicalShellPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId1.keepRows(rows);
    m_thickness1.keepRows(rows);
    m_matId2.keepRows(rows);
    m_inertia.keepRows(rows);
    m_matId3.keepRows(rows);
    m_thickness2.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_fiberDistance1.keepRows(rows);
    m_fiberDistance2.keepRows(rows);
    m_phiAngle.keepRows(rows);
}

void ConicalShellPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId1 = m_matId1.get(row); matId1 > 0)
        m_matId1.set(row, visitor.visit("matId1", ReferenceTarget::Material, matId1));
    if (const base::EntityId matId2 = m_matId2.get(row); matId2 > 0)
        m_matId2.set(row, visitor.visit("matId2", ReferenceTarget::Material, matId2));
    if (const base::EntityId matId3 = m_matId3.get(row); matId3 > 0)
        m_matId3.set(row, visitor.visit("matId3", ReferenceTarget::Material, matId3));
}

} // namespace oor::model
