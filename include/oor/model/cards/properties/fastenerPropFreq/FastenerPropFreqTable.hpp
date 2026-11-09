#pragma once

#include "oor/model/cards/properties/fastenerPropFreq/FastenerPropFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct FastenerPropFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FastenerPropFreqCard& card);
    [[nodiscard]] static std::optional<FastenerPropFreqCard> read(FieldReader& reader);
    static void write(const FastenerPropFreqCard& card, FieldWriter& writer);
};

// All the PFASTT cards. The ID space is the card type's own: the ID is the one of the PFAST that the card extends, so
// it is also a reference to a property. The two lists of tables are (N, 6) NumPy views without copy.
class FastenerPropFreqTable final : public TypedTable<FastenerPropFreqTable, FastenerPropFreqCard>
{
public:
    static constexpr std::string_view NAME = "FastenerPropFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = FastenerPropFreqFormat;

    FastenerPropFreqTable() : m_fuseStiffnessRetention(FastenerPropFreqCard::DEFAULT_FUSE_STIFFNESS_RETENTION) {}

    [[nodiscard]] static base::EntityId cardId(const FastenerPropFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const FastenerPropFreqCard& card, base::CardKey key);
    [[nodiscard]] FastenerPropFreqCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FastenerPropFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 6>>& stiffnessTableIdsColumn() noexcept
    {
        return m_stiffnessTableIds;
    }
    [[nodiscard]] Column<base::EntityId>& structuralDampingTableIdColumn() noexcept
    {
        return m_structuralDampingTableId;
    }
    [[nodiscard]] Column<std::array<base::EntityId, 6>>& nonlinearForceTableIdsColumn() noexcept
    {
        return m_nonlinearForceTableIds;
    }
    [[nodiscard]] Column<std::uint8_t>& forceDeflectionRuleColumn() noexcept { return m_forceDeflectionRule; }
    [[nodiscard]] Column<std::uint8_t>& fuseModeColumn() noexcept { return m_fuseMode; }
    [[nodiscard]] Column<std::int32_t>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] Column<std::uint8_t>& fuseOptionColumn() noexcept { return m_fuseOption; }
    [[nodiscard]] Column<std::array<double, 2>>& boundsColumn() noexcept { return m_bounds; }
    [[nodiscard]] Column<double>& fuseStiffnessRetentionColumn() noexcept { return m_fuseStiffnessRetention; }
    [[nodiscard]] Column<std::int32_t>& largeRotationEndAColumn() noexcept { return m_largeRotationEndA; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 6>> m_stiffnessTableIds;
    Column<base::EntityId> m_structuralDampingTableId;
    Column<std::array<base::EntityId, 6>> m_nonlinearForceTableIds;
    Column<std::uint8_t> m_forceDeflectionRule;
    Column<std::uint8_t> m_fuseMode;
    Column<std::int32_t> m_direction;
    Column<std::uint8_t> m_fuseOption;
    Column<std::array<double, 2>> m_bounds; // LOWER, UPPER
    Column<double> m_fuseStiffnessRetention;
    Column<std::int32_t> m_largeRotationEndA;
};

} // namespace oor::model
