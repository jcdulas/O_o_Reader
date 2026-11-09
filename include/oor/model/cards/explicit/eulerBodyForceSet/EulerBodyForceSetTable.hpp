#pragma once

#include "oor/model/cards/explicit/eulerBodyForceSet/EulerBodyForceSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerBodyForceSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerBodyForceSetCard& card);
    [[nodiscard]] static std::optional<EulerBodyForceSetCard> read(FieldReader& reader);
    static void write(const EulerBodyForceSetCard& card, FieldWriter& writer);
};

// All the EULFOR1 cards. The SID is unique among the EULFOR1 cards (IdSpace::PerCardType) and is in the category
// EulerBodyForce, shared with EULFOR. The ESID names the set of EULFREG cards (EulerRegionSet).
class EulerBodyForceSetTable final : public TypedTable<EulerBodyForceSetTable, EulerBodyForceSetCard>
{
public:
    static constexpr std::string_view NAME = "EulerBodyForceSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerBodyForce, base::IdSpace::PerCardType};
    using Format = EulerBodyForceSetFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerBodyForceSetCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerBodyForceSetCard& card, base::CardKey key);
    [[nodiscard]] EulerBodyForceSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerBodyForceSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerBodyForceSetCard> m_cards;
};

} // namespace oor::model
