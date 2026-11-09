#include "oor/model/cards/thermal/viewFactorFace/ViewFactorFaceTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

ViewFactorFaceTable::ViewFactorFaceTable() : m_shading("Both"), m_betaSubdivisions(1), m_gammaSubdivisions(1) {}

std::optional<ViewFactorFaceCard> ViewFactorFaceFormat::read(FieldReader& reader)
{
    ViewFactorFaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.cavityId = reader.integer(2, "cavityId", 0);
    card.shading = readKeyword(reader, 3, "shading", {"None", "CanShade", "CanBeShaded", "Both"}, "Both");
    card.betaSubdivisions = reader.integer(4, "betaSubdivisions", 1);
    card.gammaSubdivisions = reader.integer(5, "gammaSubdivisions", 1);
    card.surfaceOffset = reader.real(6, "surfaceOffset", 0.0);
    requireUnusedBlank(reader, {}, 6);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViewFactorFaceFormat::check(const ViewFactorFaceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the view factor ID must be a positive integer", "id"};
    if (card.cavityId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "cavityId"};
    if (const auto problem = checkKeyword(card.shading, "shading", {"None", "CanShade", "CanBeShaded", "Both"}))
        return problem;
    if (card.betaSubdivisions < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1",
                           "betaSubdivisions"};
    if (card.gammaSubdivisions < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1",
                           "gammaSubdivisions"};
    return std::nullopt;
}

void ViewFactorFaceFormat::write(const ViewFactorFaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.cavityId, 0);
    writer.textUnlessDefault(3, card.shading, "Both");
    writer.integerUnlessDefault(4, card.betaSubdivisions, 1);
    writer.integerUnlessDefault(5, card.gammaSubdivisions, 1);
    writer.realUnlessDefault(6, card.surfaceOffset, 0.0);
}

std::uint32_t ViewFactorFaceTable::append(const ViewFactorFaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cavityId.push(card.cavityId);
    m_shading.push(card.shading);
    m_betaSubdivisions.push(card.betaSubdivisions);
    m_gammaSubdivisions.push(card.gammaSubdivisions);
    m_surfaceOffset.push(card.surfaceOffset);
    return row;
}

ViewFactorFaceCard ViewFactorFaceTable::card(std::uint32_t row) const
{
    ViewFactorFaceCard card;
    card.id = m_id.get(row);
    card.cavityId = m_cavityId.get(row);
    card.shading = m_shading.get(row);
    card.betaSubdivisions = m_betaSubdivisions.get(row);
    card.gammaSubdivisions = m_gammaSubdivisions.get(row);
    card.surfaceOffset = m_surfaceOffset.get(row);
    return card;
}

void ViewFactorFaceTable::store(std::uint32_t row, const ViewFactorFaceCard& card)
{
    m_id.set(row, card.id);
    m_cavityId.set(row, card.cavityId);
    m_shading.set(row, card.shading);
    m_betaSubdivisions.set(row, card.betaSubdivisions);
    m_gammaSubdivisions.set(row, card.gammaSubdivisions);
    m_surfaceOffset.set(row, card.surfaceOffset);
}

void ViewFactorFaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cavityId.keepRows(rows);
    m_shading.keepRows(rows);
    m_betaSubdivisions.keepRows(rows);
    m_gammaSubdivisions.keepRows(rows);
    m_surfaceOffset.keepRows(rows);
}

void ViewFactorFaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId cavityId = m_cavityId.get(row); cavityId > 0)
        m_cavityId.set(row, visitor.visit("cavityId", ReferenceTarget::RadiationCavity, cavityId));
}

} // namespace oor::model
