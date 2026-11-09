#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Table.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<ContactBody1Card> ContactBody1Format::read(FieldReader& reader)
{
    ContactBody1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.bodyParamsId = reader.integer(2, "bodyParamsId", 0);
    card.dimension = readKeyword(reader, 3, "dimension", {"TwoDimensional", "ThreeDimensional"}, "ThreeDimensional");
    card.behavior =
        readKeyword(reader, 4, "behavior", {"Deformable", "Rigid", "Symmetric", "HeatTransfer"}, "Deformable");
    card.surfaceId = reader.integer(5, "surfaceId", 0);
    card.rigidMotionId = reader.integer(6, "rigidMotionId", 0);
    card.forceReferencePoint = reader.integer(7, "forceReferencePoint", 0);
    requireUnusedBlank(reader, {}, 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBody1Format::check(const ContactBody1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.bodyParamsId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of body parameters, or blank",
                           "bodyParamsId"};
    if (auto problem = checkKeyword(card.dimension, "dimension", {"TwoDimensional", "ThreeDimensional"}))
        return problem;
    if (auto problem = checkKeyword(card.behavior, "behavior", {"Deformable", "Rigid", "Symmetric", "HeatTransfer"}))
        return problem;
    if (card.surfaceId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "surfaceId"};
    // A rigid body may be described by its BCRIGID alone; any other body needs the surface it is made of.
    const bool rigid = card.behavior == "Rigid" || card.behavior == "Symmetric";
    if (card.surfaceId == 0 && !(rigid && card.rigidMotionId > 0))
        return CardProblem{"card.missing-field", "a contact body needs its " + std::string(CardProblem::FIELD),
                           "surfaceId"};
    if (card.rigidMotionId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of a rigid body motion, or blank",
                           "rigidMotionId"};
    if (card.forceReferencePoint != 0 && card.forceReferencePoint != -1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or -1",
                           "forceReferencePoint"};
    return std::nullopt;
}

void ContactBody1Format::write(const ContactBody1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.bodyParamsId, 0);
    writer.textUnlessDefault(3, card.dimension, "ThreeDimensional");
    writer.textUnlessDefault(4, card.behavior, "Deformable");
    writer.integerUnlessDefault(5, card.surfaceId, 0);
    writer.integerUnlessDefault(6, card.rigidMotionId, 0);
    writer.integerUnlessDefault(7, card.forceReferencePoint, 0);
}

std::uint32_t ContactBody1Table::append(const ContactBody1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bodyParamsId.push(card.bodyParamsId);
    m_dimension.push(card.dimension);
    m_behavior.push(card.behavior);
    m_surfaceId.push(card.surfaceId);
    m_rigidMotionId.push(card.rigidMotionId);
    m_forceReferencePoint.push(card.forceReferencePoint);
    return row;
}

ContactBody1Card ContactBody1Table::card(std::uint32_t row) const
{
    ContactBody1Card card;
    card.id = m_id.get(row);
    card.bodyParamsId = m_bodyParamsId.get(row);
    card.dimension = m_dimension.get(row);
    card.behavior = m_behavior.get(row);
    card.surfaceId = m_surfaceId.get(row);
    card.rigidMotionId = m_rigidMotionId.get(row);
    card.forceReferencePoint = m_forceReferencePoint.get(row);
    return card;
}

void ContactBody1Table::store(std::uint32_t row, const ContactBody1Card& card)
{
    m_id.set(row, card.id);
    m_bodyParamsId.set(row, card.bodyParamsId);
    m_dimension.set(row, card.dimension);
    m_behavior.set(row, card.behavior);
    m_surfaceId.set(row, card.surfaceId);
    m_rigidMotionId.set(row, card.rigidMotionId);
    m_forceReferencePoint.set(row, card.forceReferencePoint);
}

void ContactBody1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bodyParamsId.keepRows(rows);
    m_dimension.keepRows(rows);
    m_behavior.keepRows(rows);
    m_surfaceId.keepRows(rows);
    m_rigidMotionId.keepRows(rows);
    m_forceReferencePoint.keepRows(rows);
}

void ContactBody1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto visit = [&](Column<base::EntityId>& column, const char* field, ReferenceTarget target)
    {
        base::EntityId id = column.get(row);
        const base::EntityId before = id;
        visitContactId(id, visitor, field, target);
        if (id != before)
            column.set(row, id);
    };
    visit(m_bodyParamsId, "bodyParamsId", ReferenceTarget::ContactBodyParameters);
    visit(m_surfaceId, "surfaceId", ReferenceTarget::ContactSurface);
    visit(m_rigidMotionId, "rigidMotionId", ReferenceTarget::ContactRigid);
}

} // namespace oor::model
