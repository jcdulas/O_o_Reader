#pragma once

#include "oor/model/cards/dynamics/hybridDampingRotorOptions/HybridDampingRotorOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HybridDampingRotorOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HybridDampingRotorOptionsCard& card);
    [[nodiscard]] static std::optional<HybridDampingRotorOptionsCard> read(FieldReader& reader);
    static void write(const HybridDampingRotorOptionsCard& card, FieldWriter& writer);
};

// All the DAMPING cards, in the category HybridDampingRotorOptionsSpecification: the ID is unique per card type
// (IdSpace::PerCardType) and is what the Case Control RSDAMP and SEDAMP name. The HYBRID field names a HYBDAMP. The
// cards are kept whole next to a column of their IDs.
class HybridDampingRotorOptionsTable final
    : public TypedTable<HybridDampingRotorOptionsTable, HybridDampingRotorOptionsCard>
{
public:
    static constexpr std::string_view NAME = "HybridDampingRotorOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HybridDampingRotorOptionsSpecification,
                                          base::IdSpace::PerCardType};
    using Format = HybridDampingRotorOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const HybridDampingRotorOptionsCard& card) noexcept { return card.id; }
    std::uint32_t append(const HybridDampingRotorOptionsCard& card, base::CardKey key);
    [[nodiscard]] HybridDampingRotorOptionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HybridDampingRotorOptionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    // The only reference is the HYBDAMP of the HYBRID field (not looked up while it is blank).
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HybridDampingRotorOptionsCard> m_cards;
};

} // namespace oor::model
