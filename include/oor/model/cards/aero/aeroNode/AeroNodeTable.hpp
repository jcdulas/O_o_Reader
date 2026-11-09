#pragma once

#include "oor/model/cards/aero/aeroNode/AeroNodeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroNodeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroNodeCard& card);
    [[nodiscard]] static std::optional<AeroNodeCard> read(FieldReader& reader);
    static void write(const AeroNodeCard& card, FieldWriter& writer);
};

// All the AEGRID cards, in the category AeroNode that the grid points of an AEQUAD4 or an AETRIA3 name
// (ReferenceTarget::AeroNode). The GID is unique among the AEGRID cards (IdSpace::PerCardType) and has no relation to
// the structural grid points. CP and CD are coordinate systems (ReferenceTarget::Coordinate), ASID an AESCALE
// (ReferenceTarget::AeroScale). The card is kept whole next to its ID.
class AeroNodeTable final : public TypedTable<AeroNodeTable, AeroNodeCard>
{
public:
    static constexpr std::string_view NAME = "AeroNode";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroNode, base::IdSpace::PerCardType};
    using Format = AeroNodeFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroNodeCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroNodeCard& card, base::CardKey key);
    [[nodiscard]] AeroNodeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroNodeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroNodeCard> m_cards;
};

} // namespace oor::model
