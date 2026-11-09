#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 19;

} // namespace

std::optional<AeroFiniteSurfaceSpline6Card> AeroFiniteSurfaceSpline6Format::read(FieldReader& reader)
{
    AeroFiniteSurfaceSpline6Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.integer(2, "aeroPanelId", 0);
    card.aeroBoxListId = reader.requiredInteger(3, "aeroBoxListId");
    card.nodeSetId = reader.requiredInteger(5, "nodeSetId");
    card.linearFlexibility = reader.real(6, "linearFlexibility", 0.0);
    card.method =
        readKeyword<AeroFiniteSurfaceSpline6Method>(reader, 7, "method", AERO_FINITE_SURFACE_SPLINE6_METHOD_WORDS,
                                                    AeroFiniteSurfaceSpline6Method::TranslationsAndRotations);
    card.usage = readKeyword<SplineUsage>(reader, 8, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    card.surfaceKind = readKeyword<AeroFiniteSurfaceSpline6SurfaceType>(reader, 9, "surfaceKind",
                                                                        AERO_FINITE_SURFACE_SPLINE6_SURFACE_TYPE_WORDS,
                                                                        AeroFiniteSurfaceSpline6SurfaceType::Aero);
    card.surfaceElemListId = reader.integer(10, "surfaceElemListId", 0);
    card.independentPointCount = reader.integer(11, "independentPointCount", 3);
    card.dependentPointCount = reader.integer(12, "dependentPointCount", 3);
    card.surfaceMethod = readKeyword<AeroFiniteSurfaceSpline6SurfaceMethod>(
        reader, 13, "surfaceMethod", AERO_FINITE_SURFACE_SPLINE6_SURFACE_METHOD_WORDS,
        AeroFiniteSurfaceSpline6SurfaceMethod::TranslationsAndRotations);
    card.rotationFlexibility = reader.real(14, "rotationFlexibility", 0.0);
    card.connectionMethod = readKeyword<AeroFiniteSurfaceSpline6Connection>(
        reader, 15, "connectionMethod", AERO_FINITE_SURFACE_SPLINE6_CONNECTION_WORDS,
        AeroFiniteSurfaceSpline6Connection::NodalProximity);
    card.nodeCount = reader.integer(16, "nodeCount", 1);
    card.projectionTolerance = reader.real(17, "projectionTolerance", 100.0);
    card.cycleCount = reader.integer(18, "cycleCount", 3);
    card.augmentationWeight = reader.real(19, "augmentationWeight", 0.0);
    requireBlankFields(reader, {4}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroFiniteSurfaceSpline6Format::check(const AeroFiniteSurfaceSpline6Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.aeroPanelId, "aeroPanelId"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroBoxListId, "aeroBoxListId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkReal(card.linearFlexibility, "linearFlexibility", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.surfaceElemListId, "surfaceElemListId"))
        return problem;
    // The virtual surface is only implied by the CAERO when the surface is aerodynamic and there is a CAERO.
    if (card.surfaceElemListId == 0 &&
        (card.aeroPanelId == 0 || card.surfaceKind == AeroFiniteSurfaceSpline6SurfaceType::Struc))
        return CardProblem{"card.missing-field",
                           "{field} is required unless {field:aeroPanelId} is given and {field:surfaceKind} is AERO",
                           "surfaceElemListId"};
    if (card.independentPointCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "independentPointCount"};
    if (card.dependentPointCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "dependentPointCount"};
    if (const auto problem = checkReal(card.rotationFlexibility, "rotationFlexibility", RealRule::NonNegative))
        return problem;
    if (card.nodeCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "nodeCount"};
    if (const auto problem = checkReal(card.projectionTolerance, "projectionTolerance", RealRule::Finite))
        return problem;
    if (card.cycleCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "cycleCount"};
    return checkReal(card.augmentationWeight, "augmentationWeight", RealRule::NonNegative);
}

void AeroFiniteSurfaceSpline6Format::write(const AeroFiniteSurfaceSpline6Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.aeroPanelId, 0);
    writer.integer(3, card.aeroBoxListId);
    writer.integer(5, card.nodeSetId);
    writer.realUnlessDefault(6, card.linearFlexibility, 0.0);
    writeKeywordUnlessDefault(writer, 7, AERO_FINITE_SURFACE_SPLINE6_METHOD_WORDS, card.method,
                              AeroFiniteSurfaceSpline6Method::TranslationsAndRotations);
    writeKeywordUnlessDefault(writer, 8, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    writeKeywordUnlessDefault(writer, 9, AERO_FINITE_SURFACE_SPLINE6_SURFACE_TYPE_WORDS, card.surfaceKind,
                              AeroFiniteSurfaceSpline6SurfaceType::Aero);
    writer.integerUnlessDefault(10, card.surfaceElemListId, 0);
    writer.integerUnlessDefault(11, card.independentPointCount, 3);
    writer.integerUnlessDefault(12, card.dependentPointCount, 3);
    writeKeywordUnlessDefault(writer, 13, AERO_FINITE_SURFACE_SPLINE6_SURFACE_METHOD_WORDS, card.surfaceMethod,
                              AeroFiniteSurfaceSpline6SurfaceMethod::TranslationsAndRotations);
    writer.realUnlessDefault(14, card.rotationFlexibility, 0.0);
    writeKeywordUnlessDefault(writer, 15, AERO_FINITE_SURFACE_SPLINE6_CONNECTION_WORDS, card.connectionMethod,
                              AeroFiniteSurfaceSpline6Connection::NodalProximity);
    writer.integerUnlessDefault(16, card.nodeCount, 1);
    writer.realUnlessDefault(17, card.projectionTolerance, 100.0);
    writer.integerUnlessDefault(18, card.cycleCount, 3);
    writer.realUnlessDefault(19, card.augmentationWeight, 0.0);
}

std::uint32_t AeroFiniteSurfaceSpline6Table::append(const AeroFiniteSurfaceSpline6Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroFiniteSurfaceSpline6Table::store(std::uint32_t row, const AeroFiniteSurfaceSpline6Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroFiniteSurfaceSpline6Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroFiniteSurfaceSpline6Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroFiniteSurfaceSpline6Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroFiniteSurfaceSpline6Card& card = m_cards.edit(row);
    visitId(visitor, "aeroPanelId", ReferenceTarget::AeroElement, card.aeroPanelId);
    visitId(visitor, "aeroBoxListId", ReferenceTarget::AeroList, card.aeroBoxListId);
    visitId(visitor, "nodeSetId", ReferenceTarget::Set, card.nodeSetId);
    visitId(visitor, "surfaceElemListId", ReferenceTarget::AeroList, card.surfaceElemListId);
}

} // namespace oor::model
