#pragma once

#include "oor/model/cards/explicit/eulerBodyForce/EulerBodyForceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerBodyForceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerBodyForceCard& card);
    [[nodiscard]] static std::optional<EulerBodyForceCard> read(FieldReader& reader);
    static void write(const EulerBodyForceCard& card, FieldWriter& writer);
};

// All the EULFOR cards. The BID is unique among the EULFOR cards (IdSpace::PerCardType) and is in the category
// EulerBodyForce, shared with EULFOR1, which a PEULER or PEULER1 names. CID is a coordinate system, TID a TABLED1.
class EulerBodyForceTable final : public TypedTable<EulerBodyForceTable, EulerBodyForceCard>
{
public:
    static constexpr std::string_view NAME = "EulerBodyForce";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerBodyForce, base::IdSpace::PerCardType};
    using Format = EulerBodyForceFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerBodyForceCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerBodyForceCard& card, base::CardKey key);
    [[nodiscard]] EulerBodyForceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerBodyForceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerBodyForceCard> m_cards;
};

} // namespace oor::model
