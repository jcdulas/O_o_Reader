#pragma once

#include "oor/model/cards/explicit/eulerProp/EulerPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerPropCard& card);
    [[nodiscard]] static std::optional<EulerPropCard> read(FieldReader& reader);
    static void write(const EulerPropCard& card, FieldWriter& writer);
};

// All the PEULER cards. A PEULER is a property: its ID is in the space every property type shares
// (IdSpace::Property), so an element, a MESH or a HGSUPPR that names a PID finds it. Its MID is a material.
class EulerPropTable final : public TypedTable<EulerPropTable, EulerPropCard>
{
public:
    static constexpr std::string_view NAME = "EulerProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = EulerPropFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerPropCard& card, base::CardKey key);
    [[nodiscard]] EulerPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerPropCard> m_cards;
};

} // namespace oor::model
