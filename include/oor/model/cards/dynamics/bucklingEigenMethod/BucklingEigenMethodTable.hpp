#pragma once

#include "oor/model/cards/dynamics/bucklingEigenMethod/BucklingEigenMethodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BucklingEigenMethodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BucklingEigenMethodCard& card);
    [[nodiscard]] static std::optional<BucklingEigenMethodCard> read(FieldReader& reader);
    static void write(const BucklingEigenMethodCard& card, FieldWriter& writer);
};

// All the EIGB cards. The ID is a method ID, in the ID space that EIGR, EIGRL and EIGC share (IdSpace::Method). The
// point G of NORM = POINT is a grid or scalar point.
class BucklingEigenMethodTable final : public TypedTable<BucklingEigenMethodTable, BucklingEigenMethodCard>
{
public:
    static constexpr std::string_view NAME = "BucklingEigenMethod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Method, base::IdSpace::Method};
    using Format = BucklingEigenMethodFormat;

    [[nodiscard]] static base::EntityId cardId(const BucklingEigenMethodCard& card) noexcept { return card.id; }
    std::uint32_t append(const BucklingEigenMethodCard& card, base::CardKey key);
    [[nodiscard]] BucklingEigenMethodCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BucklingEigenMethodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<double>& lowerEigenvalueColumn() noexcept { return m_lowerEigenvalue; }
    [[nodiscard]] Column<double>& upperEigenvalueColumn() noexcept { return m_upperEigenvalue; }
    [[nodiscard]] Column<std::int32_t>& estimatedModeCountColumn() noexcept { return m_estimatedModeCount; }
    [[nodiscard]] Column<std::int32_t>& desiredPositiveModeCountColumn() noexcept { return m_desiredPositiveModeCount; }
    [[nodiscard]] Column<std::int32_t>& desiredNegativeModeCountColumn() noexcept { return m_desiredNegativeModeCount; }
    [[nodiscard]] Column<std::uint8_t>& normColumn() noexcept { return m_norm; }
    [[nodiscard]] Column<base::EntityId>& normNodeColumn() noexcept { return m_normNode; }
    [[nodiscard]] Column<std::int32_t>& normComponentColumn() noexcept { return m_normComponent; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_method;
    Column<double> m_lowerEigenvalue;
    Column<double> m_upperEigenvalue;
    Column<std::int32_t> m_estimatedModeCount;
    Column<std::int32_t> m_desiredPositiveModeCount;
    Column<std::int32_t> m_desiredNegativeModeCount;
    Column<std::uint8_t> m_norm;
    Column<base::EntityId> m_normNode;
    Column<std::int32_t> m_normComponent;
};

} // namespace oor::model
