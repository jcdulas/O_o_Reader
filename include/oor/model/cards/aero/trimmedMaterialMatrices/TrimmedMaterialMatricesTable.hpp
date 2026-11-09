#pragma once

#include "oor/model/cards/aero/trimmedMaterialMatrices/TrimmedMaterialMatricesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct TrimmedMaterialMatricesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TrimmedMaterialMatricesCard& card);
    [[nodiscard]] static std::optional<TrimmedMaterialMatricesCard> read(FieldReader& reader);
    static void write(const TrimmedMaterialMatricesCard& card, FieldWriter& writer);
};

// All the ACTRIM cards. An ACTRIM has no numeric ID (IdSpace::None, category Aero): the Case Control ACTRIM names it by
// its NAME, which Python uses as the key (`model.actrim["FLOOR"]`). A repeated NAME is not refused by the card: a check
// of the model reports it. The card refers to no other card (the unit numbers belong to the FMS section). The cards are
// kept whole next to a column of their names.
class TrimmedMaterialMatricesTable final : public TypedTable<TrimmedMaterialMatricesTable, TrimmedMaterialMatricesCard>
{
public:
    static constexpr std::string_view NAME = "TrimmedMaterialMatrices";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = TrimmedMaterialMatricesFormat;

    [[nodiscard]] static base::EntityId cardId(const TrimmedMaterialMatricesCard&) noexcept { return 0; }
    std::uint32_t append(const TrimmedMaterialMatricesCard& card, base::CardKey key);
    [[nodiscard]] TrimmedMaterialMatricesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TrimmedMaterialMatricesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the card refers to no other card

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<TrimmedMaterialMatricesCard> m_cards;
};

} // namespace oor::model
