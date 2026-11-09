#pragma once

#include "oor/model/cards/explicit/couplingHoleToGasBag/CouplingHoleToGasBagCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingHoleToGasBagFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingHoleToGasBagCard& card);
    [[nodiscard]] static std::optional<CouplingHoleToGasBagCard> read(FieldReader& reader);
    static void write(const CouplingHoleToGasBagCard& card, FieldWriter& writer);
};

// All the PORFGBG cards: the FID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the GBID is a GBAG and the MID a MATDEUL.
class CouplingHoleToGasBagTable final : public TypedTable<CouplingHoleToGasBagTable, CouplingHoleToGasBagCard>
{
public:
    static constexpr std::string_view NAME = "CouplingHoleToGasBag";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = CouplingHoleToGasBagFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingHoleToGasBagCard& card) noexcept { return card.id; }
    std::uint32_t append(const CouplingHoleToGasBagCard& card, base::CardKey key);
    [[nodiscard]] CouplingHoleToGasBagCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingHoleToGasBagCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CouplingHoleToGasBagCard> m_cards;
};

} // namespace oor::model
