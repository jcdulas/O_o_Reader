#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"

namespace oor::model
{

namespace
{

// The parameters of SOLs 101 and 400, then those of SOL 700. IGLUE and JGLUE are in both with the same
// kind. A table ID or a grid ID is an integer.
constexpr ContactParameterRule RULES[] = {
    // SOLs 101 and 400.
    {"AugmentationDistance", ContactParameterKind::Real},
    {"ContactToleranceBias", ContactParameterKind::Real},
    {"InterferenceClosure", ContactParameterKind::Real},
    {"TouchingBodyOptions", ContactParameterKind::Integer},
    {"TouchedBodyOptions", ContactParameterKind::Integer},
    {"ContactDistanceTolerance", ContactParameterKind::Real},
    {"HardSoftRatio", ContactParameterKind::Real},
    // The stiffness of a glued contact (the FGC parameters): a normal and a tangential stiffness, their variants for
    // the
    // rotations and for the start (integers in the format reference), and the grid points of the rotation centres.
    {"GlueStiffnessOption", ContactParameterKind::Integer},
    {"GlueNormalStiffness", ContactParameterKind::Real},
    {"GlueTangentStiffness", ContactParameterKind::Real},
    {"GlueRotationNormalStiffness", ContactParameterKind::Integer},
    {"GlueRotationTangentStiffness", ContactParameterKind::Integer},
    {"GlueInitialNormalStiffness", ContactParameterKind::Integer},
    {"GlueInitialTangentStiffness", ContactParameterKind::Integer},
    {"GlueRotationCenter", ContactParameterKind::Integer},
    {"GlueSecondRotationCenter", ContactParameterKind::Integer},
    {"StressFreeInitialContact", ContactParameterKind::Integer},
    {"GlueOption", ContactParameterKind::Integer},
    {"ContactSearchOrder", ContactParameterKind::Integer},
    {"GlueSeparationOption", ContactParameterKind::Integer},
    {"NormalPenaltyFactor", ContactParameterKind::Real},
    {"SlidingVelocityThreshold", ContactParameterKind::Real},
    {"StickSlip", ContactParameterKind::Real},
    {"TangentPenaltyFactor", ContactParameterKind::Real},
    // The interference fit (the INTRF parameters): how the closure is applied, the table that scales it, a direction
    // (with the coordinate system it is given in) or a centre.
    {"InterferenceOption", ContactParameterKind::Integer},
    {"InterferenceTable", ContactParameterKind::Integer},
    {"InterferenceBodyOption", ContactParameterKind::Integer},
    {"InterferenceDirectionSystem", ContactParameterKind::Integer},
    {"InterferenceDirectionX", ContactParameterKind::Real},
    {"InterferenceDirectionY", ContactParameterKind::Real},
    {"InterferenceDirectionZ", ContactParameterKind::Real},
    {"InterferenceTolerance", ContactParameterKind::Real},
    {"InterferenceCenterX", ContactParameterKind::Real},
    {"InterferenceCenterY", ContactParameterKind::Real},
    {"InterferenceCenterZ", ContactParameterKind::Real},
    // The initial gap (the INGP parameters): how a gap or an overlap found at the start is treated, its tolerances,
    // the coordinate system of its direction and a margin.
    {"InitialGapOption", ContactParameterKind::Integer},
    {"InitialGapTolerance", ContactParameterKind::Real},
    {"InitialGapSecondTolerance", ContactParameterKind::Real},
    {"InitialGapDirectionSystem", ContactParameterKind::Integer},
    {"InitialGapMargin", ContactParameterKind::Real},
    {"NormalPenaltyScale", ContactParameterKind::Real},
    {"TangentPenaltyScale", ContactParameterKind::Real},
    {"ContactTimeControl", ContactParameterKind::Integer},
    // SOL 700.
    {"ContactMethod", ContactParameterKind::Text},
    {"AdaptiveContactSurface", ContactParameterKind::Text},
    {"ContactThickness", ContactParameterKind::Real},
    {"ThicknessOffset", ContactParameterKind::Real},
    {"PenetrationLimit", ContactParameterKind::Real},
    {"MaxParametricCoordinate", ContactParameterKind::Real},
    {"SoftContact", ContactParameterKind::Integer},
    {"IgnoreInitialPenetration", ContactParameterKind::Integer},
    {"AutomaticContact", ContactParameterKind::Text},
    {"ContactSide", ContactParameterKind::Text},
    {"ContactForceWeighting", ContactParameterKind::Text},
    {"MonitorDisplacement", ContactParameterKind::Text},
    {"MonitorDisplacementValue", ContactParameterKind::Real},
    {"InitialMonitor", ContactParameterKind::Real},
};

// The ID (BCGPID) is field 1 and field 2 is blank: the first pair is at field 3.
constexpr int FIRST_PAIR_FIELD = 3;

} // namespace

std::span<const ContactParameterRule> ContactPairGeomParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactPairGeomParamsCard> ContactPairGeomParamsFormat::read(FieldReader& reader)
{
    ContactPairGeomParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is blank: the first parameter is in field 3",
                     "parameterName");
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactPairGeomParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactPairGeomParamsFormat::check(const ContactPairGeomParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    return checkContactParameters(card.params, RULES, ContactPairGeomParamsTable::NAME);
}

void ContactPairGeomParamsFormat::write(const ContactPairGeomParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactPairGeomParamsTable::NAME);
}

std::uint32_t ContactPairGeomParamsTable::append(const ContactPairGeomParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactPairGeomParamsTable::store(std::uint32_t row, const ContactPairGeomParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactPairGeomParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactPairGeomParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
