#include "oor/model/cards/thermal/viewFactorIntegration/ViewFactorIntegrationTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

bool isGaussOrder(int order) noexcept
{
    return order == 2 || order == 3 || order == 4 || order == 5 || order == 6 || order == 10;
}

} // namespace

ViewFactorIntegrationTable::ViewFactorIntegrationTable()
    : m_blockingIntegrationOrder(4),
      m_shadowingIntegrationOrder(4),
      m_contourIntegrationLevel(4),
      m_errorTolerance(0.1),
      m_zeroTolerance(1.0e-10),
      m_warpTolerance(0.01),
      m_radiationCheck(3)
{
}

std::optional<ViewFactorIntegrationCard> ViewFactorIntegrationFormat::read(FieldReader& reader)
{
    ViewFactorIntegrationCard card;
    card.cavityId = reader.requiredInteger(1, "cavityId");
    card.blockingIntegrationOrder = reader.integer(2, "blockingIntegrationOrder", 4);
    card.shadowingIntegrationOrder = reader.integer(3, "shadowingIntegrationOrder", 4);
    card.contourIntegrationLevel = reader.integer(4, "contourIntegrationLevel", 4);
    card.errorTolerance = reader.real(5, "errorTolerance", 0.1);
    card.zeroTolerance = reader.real(6, "zeroTolerance", 1.0e-10);
    card.warpTolerance = reader.real(7, "warpTolerance", 0.01);
    card.radiationCheck = reader.integer(8, "radiationCheck", 3);
    requireUnusedBlank(reader, {}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViewFactorIntegrationFormat::check(const ViewFactorIntegrationCard& card)
{
    if (card.cavityId <= 0)
        return CardProblem{"card.bad-id", "the cavity ID must be a positive integer", "cavityId"};
    if (!isGaussOrder(card.blockingIntegrationOrder))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 2, 3, 4, 5, 6 or 10",
                           "blockingIntegrationOrder"};
    if (!isGaussOrder(card.shadowingIntegrationOrder))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 2, 3, 4, 5, 6 or 10",
                           "shadowingIntegrationOrder"};
    if (card.contourIntegrationLevel < 1 || card.contourIntegrationLevel > 20)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 1 and 20",
                           "contourIntegrationLevel"};
    if (!(card.errorTolerance > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "errorTolerance"};
    if (!(card.zeroTolerance > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "zeroTolerance"};
    if (!(card.warpTolerance > 0.0 && card.warpTolerance < 1.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0.0 and 1.0",
                           "warpTolerance"};
    return std::nullopt;
}

void ViewFactorIntegrationFormat::write(const ViewFactorIntegrationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.cavityId);
    writer.integerUnlessDefault(2, card.blockingIntegrationOrder, 4);
    writer.integerUnlessDefault(3, card.shadowingIntegrationOrder, 4);
    writer.integerUnlessDefault(4, card.contourIntegrationLevel, 4);
    writer.realUnlessDefault(5, card.errorTolerance, 0.1);
    writer.realUnlessDefault(6, card.zeroTolerance, 1.0e-10);
    writer.realUnlessDefault(7, card.warpTolerance, 0.01);
    writer.integerUnlessDefault(8, card.radiationCheck, 3);
}

std::uint32_t ViewFactorIntegrationTable::append(const ViewFactorIntegrationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cavityId.push(card.cavityId);
    m_blockingIntegrationOrder.push(card.blockingIntegrationOrder);
    m_shadowingIntegrationOrder.push(card.shadowingIntegrationOrder);
    m_contourIntegrationLevel.push(card.contourIntegrationLevel);
    m_errorTolerance.push(card.errorTolerance);
    m_zeroTolerance.push(card.zeroTolerance);
    m_warpTolerance.push(card.warpTolerance);
    m_radiationCheck.push(card.radiationCheck);
    return row;
}

ViewFactorIntegrationCard ViewFactorIntegrationTable::card(std::uint32_t row) const
{
    ViewFactorIntegrationCard card;
    card.cavityId = m_cavityId.get(row);
    card.blockingIntegrationOrder = m_blockingIntegrationOrder.get(row);
    card.shadowingIntegrationOrder = m_shadowingIntegrationOrder.get(row);
    card.contourIntegrationLevel = m_contourIntegrationLevel.get(row);
    card.errorTolerance = m_errorTolerance.get(row);
    card.zeroTolerance = m_zeroTolerance.get(row);
    card.warpTolerance = m_warpTolerance.get(row);
    card.radiationCheck = m_radiationCheck.get(row);
    return card;
}

void ViewFactorIntegrationTable::store(std::uint32_t row, const ViewFactorIntegrationCard& card)
{
    m_cavityId.set(row, card.cavityId);
    m_blockingIntegrationOrder.set(row, card.blockingIntegrationOrder);
    m_shadowingIntegrationOrder.set(row, card.shadowingIntegrationOrder);
    m_contourIntegrationLevel.set(row, card.contourIntegrationLevel);
    m_errorTolerance.set(row, card.errorTolerance);
    m_zeroTolerance.set(row, card.zeroTolerance);
    m_warpTolerance.set(row, card.warpTolerance);
    m_radiationCheck.set(row, card.radiationCheck);
}

void ViewFactorIntegrationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_cavityId.keepRows(rows);
    m_blockingIntegrationOrder.keepRows(rows);
    m_shadowingIntegrationOrder.keepRows(rows);
    m_contourIntegrationLevel.keepRows(rows);
    m_errorTolerance.keepRows(rows);
    m_zeroTolerance.keepRows(rows);
    m_warpTolerance.keepRows(rows);
    m_radiationCheck.keepRows(rows);
}

} // namespace oor::model
