#pragma once

#include "oor/model/cards/explicit/standardInflator/StandardInflatorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct StandardInflatorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const StandardInflatorCard& card);
    [[nodiscard]] static std::optional<StandardInflatorCard> read(FieldReader& reader);
    static void write(const StandardInflatorCard& card, FieldWriter& writer);
};

// All the INFLTR cards. The INFLID is unique among the INFLTR cards (IdSpace::PerCardType, category Inflator: INFLCG,
// INFLHB and INFLTNK share the category, because an ABINFL names one of the four by number). The tables are TABLED1
// cards, GASNAM may name an INFLGAS and MID a MATDEUL.
class StandardInflatorTable final : public TypedTable<StandardInflatorTable, StandardInflatorCard>
{
public:
    static constexpr std::string_view NAME = "StandardInflator";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Inflator, base::IdSpace::PerCardType};
    using Format = StandardInflatorFormat;

    [[nodiscard]] static base::EntityId cardId(const StandardInflatorCard& card) noexcept { return card.id; }
    std::uint32_t append(const StandardInflatorCard& card, base::CardKey key);
    [[nodiscard]] StandardInflatorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const StandardInflatorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<StandardInflatorCard> m_cards;
};

} // namespace oor::model
