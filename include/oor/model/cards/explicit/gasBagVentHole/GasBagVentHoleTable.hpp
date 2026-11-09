#pragma once

#include "oor/model/cards/explicit/gasBagVentHole/GasBagVentHoleCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagVentHoleFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagVentHoleCard& card);
    [[nodiscard]] static std::optional<GasBagVentHoleCard> read(FieldReader& reader);
    static void write(const GasBagVentHoleCard& card, FieldWriter& writer);
};

// All the PORHOLE cards: the FID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the MID is a MATDEUL.
class GasBagVentHoleTable final : public TypedTable<GasBagVentHoleTable, GasBagVentHoleCard>
{
public:
    static constexpr std::string_view NAME = "GasBagVentHole";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = GasBagVentHoleFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagVentHoleCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagVentHoleCard& card, base::CardKey key);
    [[nodiscard]] GasBagVentHoleCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagVentHoleCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagVentHoleCard> m_cards;
};

} // namespace oor::model
