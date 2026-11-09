#pragma once

#include "oor/model/cards/explicit/gasBagRadiation/GasBagRadiationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagRadiationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagRadiationCard& card);
    [[nodiscard]] static std::optional<GasBagRadiationCard> read(FieldReader& reader);
    static void write(const GasBagRadiationCard& card, FieldWriter& writer);
};

// All the HTRRAD cards. The HTRID is the ID of the table (IdSpace::PerCardType, category HeatTransferModel): a HEATLOS
// names it, in the same category as an HTRCONV, which has a number space of its own. The emissivity may be a TABLED1.
class GasBagRadiationTable final : public TypedTable<GasBagRadiationTable, GasBagRadiationCard>
{
public:
    static constexpr std::string_view NAME = "GasBagRadiation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HeatTransferModel, base::IdSpace::PerCardType};
    using Format = GasBagRadiationFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagRadiationCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagRadiationCard& card, base::CardKey key);
    [[nodiscard]] GasBagRadiationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagRadiationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagRadiationCard> m_cards;
};

} // namespace oor::model
