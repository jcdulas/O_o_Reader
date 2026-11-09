#pragma once

#include "oor/model/cards/properties/bushProp2DFreq/BushProp2DFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BushProp2DFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BushProp2DFreqCard& card);
    [[nodiscard]] static std::optional<BushProp2DFreqCard> read(FieldReader& reader);
    static void write(const BushProp2DFreqCard& card, FieldWriter& writer);
};

// All the PBSH2DT cards. The ID space is the card type's own: the ID is the one of the PBUSH2D that the card extends
// (one PBSH2DT per PBUSH2D), so it is also a reference to a property, as for the PBUSHT. The three groups of terms are
// (N, 4) NumPy views without copy; the tables are TABLED1 entries (ReferenceTarget::DynamicTable).
class BushProp2DFreqTable final : public TypedTable<BushProp2DFreqTable, BushProp2DFreqCard>
{
public:
    static constexpr std::string_view NAME = "BushProp2DFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = BushProp2DFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const BushProp2DFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const BushProp2DFreqCard& card, base::CardKey key);
    [[nodiscard]] BushProp2DFreqCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BushProp2DFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>>& stiffnessTableIdsColumn() noexcept
    {
        return m_stiffnessTableIds;
    }
    [[nodiscard]] Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>>& dampingTableIdsColumn() noexcept
    {
        return m_dampingTableIds;
    }
    [[nodiscard]] Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>>& massTableIdsColumn() noexcept
    {
        return m_massTableIds;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>> m_stiffnessTableIds;
    Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>> m_dampingTableIds;
    Column<std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>> m_massTableIds;
};

} // namespace oor::model
