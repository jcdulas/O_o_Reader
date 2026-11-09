#pragma once

#include "oor/model/cards/explicit/gasBagConvection/GasBagConvectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagConvectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagConvectionCard& card);
    [[nodiscard]] static std::optional<GasBagConvectionCard> read(FieldReader& reader);
    static void write(const GasBagConvectionCard& card, FieldWriter& writer);
};

// All the HTRCONV cards. The HTRID is the ID of the table (IdSpace::PerCardType, category HeatTransferModel): a HEATLOS
// names it, in the same category as an HTRRAD, which has a number space of its own. The coefficient may be a TABLED1.
class GasBagConvectionTable final : public TypedTable<GasBagConvectionTable, GasBagConvectionCard>
{
public:
    static constexpr std::string_view NAME = "GasBagConvection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HeatTransferModel, base::IdSpace::PerCardType};
    using Format = GasBagConvectionFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagConvectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagConvectionCard& card, base::CardKey key);
    [[nodiscard]] GasBagConvectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagConvectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagConvectionCard> m_cards;
};

} // namespace oor::model
