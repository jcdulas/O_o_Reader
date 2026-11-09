#pragma once

#include "oor/model/cards/explicit/inflatorGas/InflatorGasCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InflatorGasFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InflatorGasCard& card);
    [[nodiscard]] static std::optional<InflatorGasCard> read(FieldReader& reader);
    static void write(const InflatorGasCard& card, FieldWriter& writer);
};

// All the INFLGAS cards. The GASNAM is unique among the gases (IdSpace::PerCardType, category InflatorGas): an INFLCG,
// INFLTR, INFLHB or INITGAS names it. The specific heat may be a TABLED1 of the temperature.
class InflatorGasTable final : public TypedTable<InflatorGasTable, InflatorGasCard>
{
public:
    static constexpr std::string_view NAME = "InflatorGas";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InflatorGas, base::IdSpace::PerCardType};
    using Format = InflatorGasFormat;

    [[nodiscard]] static base::EntityId cardId(const InflatorGasCard& card) noexcept { return card.id; }
    std::uint32_t append(const InflatorGasCard& card, base::CardKey key);
    [[nodiscard]] InflatorGasCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InflatorGasCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InflatorGasCard> m_cards;
};

} // namespace oor::model
