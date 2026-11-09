#include "oor/model/cards/explicit/eulerTimeFlowBoundary/EulerTimeFlowBoundaryTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int BOX_FIELD = 9;
constexpr int MAT_ID_FIELD = 25;
constexpr int LAST_FIELD = 29;

} // namespace

std::optional<EulerTimeFlowBoundaryCard> EulerTimeFlowBoundaryFormat::read(FieldReader& reader)
{
    EulerTimeFlowBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.contactBodyId = reader.integer(2, "contactBodyId", 0);
    card.type = readRequiredKeyword<FlowBoundaryType>(reader, 3, "flowType", FLOW_BOUNDARY_TYPE_WORDS);
    card.meshId = reader.integer(4, "meshId", 0);
    card.meshFace = readOptionalKeyword<BoundaryFace>(reader, 5, "meshFace", BOUNDARY_FACE_WORDS);
    card.box = readBoundaryBox(reader, BOX_FIELD);
    card.velocity = readSourcedValue(reader, EULER_TIME_FLOW_VELOCITY_LAYOUT);
    card.pressure = readSourcedValue(reader, EULER_TIME_FLOW_PRESSURE_LAYOUT);
    card.matId = reader.integer(MAT_ID_FIELD, "matId", 0);
    card.density = readSourcedValue(reader, EULER_TIME_FLOW_DENSITY_LAYOUT);
    card.specificEnergy = readSourcedValue(reader, EULER_TIME_FLOW_ENERGY_LAYOUT);
    requireBlankFields(reader, {6, 7, 8, 15, 16, 21, 22, 23, 24}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerTimeFlowBoundaryFormat::check(const EulerTimeFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.contactBodyId, "contactBodyId"))
        return problem;
    if (const auto problem = checkOptionalId(card.meshId, "meshId"))
        return problem;
    if (const auto problem = checkBoundaryBox(card.box))
        return problem;
    if (const auto problem = checkSourcedValue(card.velocity, EULER_TIME_FLOW_VELOCITY_LAYOUT, RealRule::Finite))
        return problem;
    if (const auto problem = checkSourcedValue(card.pressure, EULER_TIME_FLOW_PRESSURE_LAYOUT, RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkSourcedValue(card.density, EULER_TIME_FLOW_DENSITY_LAYOUT, RealRule::NonNegative))
        return problem;
    if (const auto problem = checkSourcedValue(card.specificEnergy, EULER_TIME_FLOW_ENERGY_LAYOUT, RealRule::Finite))
        return problem;
    if (card.matId != 0 && card.type == FlowBoundaryType::Out)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is not allowed on an OUT boundary",
                           "matId"};
    if (card.type == FlowBoundaryType::Both)
    {
        const char* message = "only the pressure can be given on a BOTH boundary";
        if (card.velocity.source != ValueSource::Element)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be ELEMENT: " + message,
                               "velocitySource"};
        if (card.matId != 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be blank: " + message,
                               "matId"};
        if (card.density.source != ValueSource::Element)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be ELEMENT: " + message,
                               "densitySource"};
        if (card.specificEnergy.source != ValueSource::Element)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be ELEMENT: " + message,
                               "energySource"};
    }
    if (card.matId != 0 && card.density.source == ValueSource::Element)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be CONSTANT or TABLE when " +
                               CardProblem::fieldNamed("matId") + " is given",
                           "densitySource"};
    if (card.matId != 0 && card.specificEnergy.source == ValueSource::Element)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be CONSTANT or TABLE when " +
                               CardProblem::fieldNamed("matId") + " is given",
                           "energySource"};
    return std::nullopt;
}

void EulerTimeFlowBoundaryFormat::write(const EulerTimeFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.contactBodyId, 0);
    writeKeyword(writer, 3, FLOW_BOUNDARY_TYPE_WORDS, card.type);
    writer.integerUnlessDefault(4, card.meshId, 0);
    writeOptionalKeyword(writer, 5, BOUNDARY_FACE_WORDS, card.meshFace);
    writeBoundaryBox(writer, BOX_FIELD, card.box);
    writeSourcedValue(writer, EULER_TIME_FLOW_VELOCITY_LAYOUT, card.velocity);
    writeSourcedValue(writer, EULER_TIME_FLOW_PRESSURE_LAYOUT, card.pressure);
    writer.integerUnlessDefault(MAT_ID_FIELD, card.matId, 0);
    writeSourcedValue(writer, EULER_TIME_FLOW_DENSITY_LAYOUT, card.density);
    writeSourcedValue(writer, EULER_TIME_FLOW_ENERGY_LAYOUT, card.specificEnergy);
}

std::uint32_t EulerTimeFlowBoundaryTable::append(const EulerTimeFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerTimeFlowBoundaryTable::store(std::uint32_t row, const EulerTimeFlowBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerTimeFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerTimeFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerTimeFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerTimeFlowBoundaryCard& card = m_cards.edit(row);
    visitId(visitor, "contactBodyId", ReferenceTarget::ContactSurface, card.contactBodyId);
    visitId(visitor, "meshId", ReferenceTarget::EulerMesh, card.meshId);
    visitSourcedTable(visitor, "velocity", card.velocity);
    visitSourcedTable(visitor, "pressure", card.pressure);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    visitSourcedTable(visitor, "density", card.density);
    visitSourcedTable(visitor, "specificEnergy", card.specificEnergy);
}

} // namespace oor::model
