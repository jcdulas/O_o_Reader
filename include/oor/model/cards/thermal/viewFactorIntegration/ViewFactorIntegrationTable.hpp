#pragma once

#include "oor/model/cards/thermal/viewFactorIntegration/ViewFactorIntegrationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ViewFactorIntegrationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViewFactorIntegrationCard& card);
    [[nodiscard]] static std::optional<ViewFactorIntegrationCard> read(FieldReader& reader);
    static void write(const ViewFactorIntegrationCard& card, FieldWriter& writer);
};

// All the VIEW3D cards. The ID is the ICAVITY, the one the RADCAV, RADLST and RADMTX of the cavity share (category
// RadiationCavity, IdSpace::PerCardType: one VIEW3D per cavity).
class ViewFactorIntegrationTable final : public TypedTable<ViewFactorIntegrationTable, ViewFactorIntegrationCard>
{
public:
    static constexpr std::string_view NAME = "ViewFactorIntegration";
    static inline const CardTypeInfo INFO{NAME, CardCategory::RadiationCavity, base::IdSpace::PerCardType};
    using Format = ViewFactorIntegrationFormat;

    ViewFactorIntegrationTable();

    [[nodiscard]] static base::EntityId cardId(const ViewFactorIntegrationCard& card) noexcept { return card.cavityId; }
    std::uint32_t append(const ViewFactorIntegrationCard& card, base::CardKey key);
    [[nodiscard]] ViewFactorIntegrationCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ViewFactorIntegrationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_cavityId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_cavityId.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // its ID is the cavity it describes

    [[nodiscard]] Column<base::EntityId>& cavityIdColumn() noexcept { return m_cavityId; }
    [[nodiscard]] Column<std::int32_t>& blockingIntegrationOrderColumn() noexcept { return m_blockingIntegrationOrder; }
    [[nodiscard]] Column<std::int32_t>& shadowingIntegrationOrderColumn() noexcept
    {
        return m_shadowingIntegrationOrder;
    }
    [[nodiscard]] Column<std::int32_t>& contourIntegrationLevelColumn() noexcept { return m_contourIntegrationLevel; }
    [[nodiscard]] Column<double>& errorToleranceColumn() noexcept { return m_errorTolerance; }
    [[nodiscard]] Column<double>& zeroToleranceColumn() noexcept { return m_zeroTolerance; }
    [[nodiscard]] Column<double>& warpToleranceColumn() noexcept { return m_warpTolerance; }
    [[nodiscard]] Column<std::int32_t>& radiationCheckColumn() noexcept { return m_radiationCheck; }

private:
    Column<base::EntityId> m_cavityId;
    Column<std::int32_t> m_blockingIntegrationOrder;
    Column<std::int32_t> m_shadowingIntegrationOrder;
    Column<std::int32_t> m_contourIntegrationLevel;
    Column<double> m_errorTolerance;
    Column<double> m_zeroTolerance;
    Column<double> m_warpTolerance;
    Column<std::int32_t> m_radiationCheck;
};

} // namespace oor::model
