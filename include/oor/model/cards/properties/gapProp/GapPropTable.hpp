#pragma once

#include "oor/model/cards/properties/gapProp/GapPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GapPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GapPropCard& card);
    [[nodiscard]] static std::optional<GapPropCard> read(FieldReader& reader);
    static void write(const GapPropCard& card, FieldWriter& writer);
};

class GapPropTable final : public TypedTable<GapPropTable, GapPropCard>
{
public:
    static constexpr std::string_view NAME = "GapProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = GapPropFormat;

    GapPropTable()
        : m_closedStiffness(GapPropCard::DEFAULT_CLOSED_STIFFNESS),
          m_maxAdjustmentRatio(GapPropCard::DEFAULT_MAX_ADJUSTMENT_RATIO),
          m_minPenetrationRatio(GapPropCard::DEFAULT_MIN_PENETRATION_RATIO)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const GapPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const GapPropCard& card, base::CardKey key);
    [[nodiscard]] GapPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GapPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& initialOpeningColumn() noexcept { return m_initialOpening; }
    [[nodiscard]] Column<double>& preloadColumn() noexcept { return m_preload; }
    [[nodiscard]] Column<double>& closedStiffnessColumn() noexcept { return m_closedStiffness; }
    [[nodiscard]] Column<double>& openStiffnessColumn() noexcept { return m_openStiffness; }
    [[nodiscard]] Column<double>& transverseStiffnessColumn() noexcept { return m_transverseStiffness; }
    [[nodiscard]] Column<double>& frictionCoeff1Column() noexcept { return m_frictionCoeff1; }
    [[nodiscard]] Column<double>& frictionCoeff2Column() noexcept { return m_frictionCoeff2; }
    [[nodiscard]] Column<double>& maxPenetrationColumn() noexcept { return m_maxPenetration; }
    [[nodiscard]] Column<double>& maxAdjustmentRatioColumn() noexcept { return m_maxAdjustmentRatio; }
    [[nodiscard]] Column<double>& minPenetrationRatioColumn() noexcept { return m_minPenetrationRatio; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_initialOpening;
    Column<double> m_preload;
    Column<double> m_closedStiffness;
    Column<double> m_openStiffness;
    Column<double> m_transverseStiffness;
    Column<double> m_frictionCoeff1;
    Column<double> m_frictionCoeff2;
    Column<double> m_maxPenetration;
    Column<double> m_maxAdjustmentRatio;
    Column<double> m_minPenetrationRatio;
};

} // namespace oor::model
