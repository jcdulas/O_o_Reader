#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"

namespace oor::model
{

namespace
{

// The parameters of the card. A table ID (the FGC... ones) is an integer.
constexpr ContactParameterRule RULES[] = {
    {"ContactDistanceTolerance", ContactParameterKind::Real},
    {"ContactToleranceBias", ContactParameterKind::Real},
    {"GlueStiffnessOption", ContactParameterKind::Integer},
    {"GlueNormalStiffness", ContactParameterKind::Real},
    {"GlueTangentStiffness", ContactParameterKind::Real},
    {"GlueRotationNormalStiffness", ContactParameterKind::Integer},
    {"GlueRotationTangentStiffness", ContactParameterKind::Integer},
    {"GlueInitialNormalStiffness", ContactParameterKind::Integer},
    {"GlueInitialTangentStiffness", ContactParameterKind::Integer},
    {"GlueRotationCenter", ContactParameterKind::Integer},
    {"GlueSecondRotationCenter", ContactParameterKind::Integer},
    {"SeparationForce", ContactParameterKind::Real},
    {"MaxSeparations", ContactParameterKind::Integer},
    {"InitialSeparationCheck", ContactParameterKind::Integer},
    {"SeparationCriterion", ContactParameterKind::Integer},
    {"ResidualVectorConstant", ContactParameterKind::Real},
    {"FrictionKind", ContactParameterKind::Integer},
    {"BeamToBeamContact", ContactParameterKind::Integer},
    {"NonlinearGlue", ContactParameterKind::Integer},
    {"NodeSeparation", ContactParameterKind::Integer},
    {"ContactMethod", ContactParameterKind::Text},
    {"AugmentationOption", ContactParameterKind::Integer},
    {"NormalPenaltyFactor", ContactParameterKind::Real},
    {"AugmentationDistance", ContactParameterKind::Real},
    {"SolidLimit", ContactParameterKind::Real},
    {"SegmentSymmetry", ContactParameterKind::Integer},
    {"ContactToleranceBasis", ContactParameterKind::Integer},
    {"DisplacementIncrementLimit", ContactParameterKind::Real},
    {"AugmentationTime", ContactParameterKind::Integer},
    {"TangentPenaltyFactor", ContactParameterKind::Real},
    {"StickSlip", ContactParameterKind::Real},
    {"IgnoreShellThickness", ContactParameterKind::Integer},
    {"LineContact", ContactParameterKind::Integer},
    {"NormalPenaltyScale", ContactParameterKind::Real},
    {"TangentPenaltyScale", ContactParameterKind::Real},
    {"SegmentAngle", ContactParameterKind::Real},
    {"ContactTimeControl", ContactParameterKind::Integer},
    {"ContactAlgorithmVersion", ContactParameterKind::Integer},
    {"BackupControl", ContactParameterKind::Integer},
    {"GlueOutput", ContactParameterKind::Integer},
    {"SeparationAcceleration", ContactParameterKind::Integer},
    {"DynamicPressureFactor", ContactParameterKind::Real},
    {"OutputSplineFreq", ContactParameterKind::Integer},
};

// The first pair of the card is at field 2 (the ID is field 1).
constexpr int FIRST_PAIR_FIELD = 2;

} // namespace

std::span<const ContactParameterRule> ContactParamsFormat::rules()
{
    return RULES;
}

std::optional<ContactParamsCard> ContactParamsFormat::read(FieldReader& reader)
{
    ContactParamsCard card;
    card.id = reader.integer(1, "id", 0);
    card.params = readContactParameters(reader, FIRST_PAIR_FIELD, RULES, ContactParamsTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactParamsFormat::check(const ContactParamsCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "id"};
    return checkContactParameters(card.params, RULES, ContactParamsTable::NAME);
}

void ContactParamsFormat::write(const ContactParamsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.id, 0);
    writeContactParameters(writer, FIRST_PAIR_FIELD, card.params, ContactParamsTable::NAME);
}

std::uint32_t ContactParamsTable::append(const ContactParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactParamsTable::store(std::uint32_t row, const ContactParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
