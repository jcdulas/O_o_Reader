#pragma once

#include "oor/model/cards/explicit/gasBagPermeableLink/GasBagPermeableLinkCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagPermeableLinkFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagPermeableLinkCard& card);
    [[nodiscard]] static std::optional<GasBagPermeableLinkCard> read(FieldReader& reader);
    static void write(const GasBagPermeableLinkCard& card, FieldWriter& writer);
};

// All the PERMGBG cards: the PID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the GBID is a GBAG.
class GasBagPermeableLinkTable final : public TypedTable<GasBagPermeableLinkTable, GasBagPermeableLinkCard>
{
public:
    static constexpr std::string_view NAME = "GasBagPermeableLink";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = GasBagPermeableLinkFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagPermeableLinkCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagPermeableLinkCard& card, base::CardKey key);
    [[nodiscard]] GasBagPermeableLinkCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagPermeableLinkCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagPermeableLinkCard> m_cards;
};

} // namespace oor::model
