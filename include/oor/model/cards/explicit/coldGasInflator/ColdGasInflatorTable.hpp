#pragma once

#include "oor/model/cards/explicit/coldGasInflator/ColdGasInflatorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ColdGasInflatorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ColdGasInflatorCard& card);
    [[nodiscard]] static std::optional<ColdGasInflatorCard> read(FieldReader& reader);
    static void write(const ColdGasInflatorCard& card, FieldWriter& writer);
};

// All the INFLCG cards. The INFLID is unique among the INFLCG cards (IdSpace::PerCardType, category Inflator: INFLHB,
// INFLTNK and INFLTR share the category, because an ABINFL names one of the four by number). GASNAM may name an
// INFLGAS.
class ColdGasInflatorTable final : public TypedTable<ColdGasInflatorTable, ColdGasInflatorCard>
{
public:
    static constexpr std::string_view NAME = "ColdGasInflator";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Inflator, base::IdSpace::PerCardType};
    using Format = ColdGasInflatorFormat;

    [[nodiscard]] static base::EntityId cardId(const ColdGasInflatorCard& card) noexcept { return card.id; }
    std::uint32_t append(const ColdGasInflatorCard& card, base::CardKey key);
    [[nodiscard]] ColdGasInflatorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ColdGasInflatorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ColdGasInflatorCard> m_cards;
};

} // namespace oor::model
