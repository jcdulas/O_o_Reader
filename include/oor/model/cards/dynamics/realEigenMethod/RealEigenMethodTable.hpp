#pragma once

#include "oor/model/cards/dynamics/realEigenMethod/RealEigenMethodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace oor::model
{

struct RealEigenMethodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RealEigenMethodCard& card);
    [[nodiscard]] static std::optional<RealEigenMethodCard> read(FieldReader& reader);
    static void write(const RealEigenMethodCard& card, FieldWriter& writer);
};

// All the EIGR cards. The ID is a method ID, in the ID space that EIGRL shares (IdSpace::Method). A blank F1 or F2 is
// NaN in its column, a blank ND is NO_DESIRED_MODE_COUNT.
class RealEigenMethodTable final : public TypedTable<RealEigenMethodTable, RealEigenMethodCard>
{
public:
    static constexpr std::string_view NAME = "RealEigenMethod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Method, base::IdSpace::Method};
    // A blank ND in its column (an ND given is 0 or more).
    static constexpr std::int32_t NO_DESIRED_MODE_COUNT = std::numeric_limits<std::int32_t>::min();
    using Format = RealEigenMethodFormat;

    [[nodiscard]] static base::EntityId cardId(const RealEigenMethodCard& card) noexcept { return card.id; }
    std::uint32_t append(const RealEigenMethodCard& card, base::CardKey key);
    [[nodiscard]] RealEigenMethodCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RealEigenMethodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<double>& frequency1Column() noexcept { return m_frequency1; }
    [[nodiscard]] Column<double>& frequency2Column() noexcept { return m_frequency2; }
    [[nodiscard]] Column<std::int32_t>& estimatedModeCountColumn() noexcept { return m_estimatedModeCount; }
    [[nodiscard]] Column<std::int32_t>& desiredModeCountColumn() noexcept { return m_desiredModeCount; }
    [[nodiscard]] Column<std::uint8_t>& normalizationColumn() noexcept { return m_normalization; }
    [[nodiscard]] Column<base::EntityId>& normNodeColumn() noexcept { return m_normNode; }
    [[nodiscard]] Column<std::int32_t>& normComponentColumn() noexcept { return m_normComponent; }

private:
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_method;
    Column<double> m_frequency1{BLANK};
    Column<double> m_frequency2{BLANK};
    Column<std::int32_t> m_estimatedModeCount;
    Column<std::int32_t> m_desiredModeCount{NO_DESIRED_MODE_COUNT};
    Column<std::uint8_t> m_normalization;
    Column<base::EntityId> m_normNode;
    Column<std::int32_t> m_normComponent;
};

} // namespace oor::model
