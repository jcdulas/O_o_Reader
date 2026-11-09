#pragma once

#include "oor/model/cards/explicit/tankTestInflator/TankTestInflatorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TankTestInflatorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TankTestInflatorCard& card);
    [[nodiscard]] static std::optional<TankTestInflatorCard> read(FieldReader& reader);
    static void write(const TankTestInflatorCard& card, FieldWriter& writer);
};

// All the INFLTNK cards. The INFLID is unique among the INFLTNK cards (IdSpace::PerCardType, category Inflator: INFLCG,
// INFLHB and INFLTR share the category, because an ABINFL names one of the four by number). TPTABLE and IPTABLE are
// TABLED1 cards.
class TankTestInflatorTable final : public TypedTable<TankTestInflatorTable, TankTestInflatorCard>
{
public:
    static constexpr std::string_view NAME = "TankTestInflator";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Inflator, base::IdSpace::PerCardType};
    using Format = TankTestInflatorFormat;

    [[nodiscard]] static base::EntityId cardId(const TankTestInflatorCard& card) noexcept { return card.id; }
    std::uint32_t append(const TankTestInflatorCard& card, base::CardKey key);
    [[nodiscard]] TankTestInflatorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TankTestInflatorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TankTestInflatorCard> m_cards;
};

} // namespace oor::model
