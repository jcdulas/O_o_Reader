#pragma once

#include "oor/model/cards/constraints/freeBodySupportSet/FreeBodySupportSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct FreeBodySupportSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeBodySupportSetCard& card);
    [[nodiscard]] static std::optional<FreeBodySupportSetCard> read(FieldReader& reader);
    static void write(const FreeBodySupportSetCard& card, FieldWriter& writer);
};

// All the SUPORT1 cards. The ID is a set ID shared by many cards (IdSpace::Group), in the Constraint category: the
// SUPORT1 Case Control command selects it. The points and the components of every card are two lists in CSR form.
class FreeBodySupportSetTable final : public TypedTable<FreeBodySupportSetTable, FreeBodySupportSetCard>
{
public:
    static constexpr std::string_view NAME = "FreeBodySupportSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = FreeBodySupportSetFormat;

    [[nodiscard]] static base::EntityId cardId(const FreeBodySupportSetCard& card) noexcept { return card.id; }
    std::uint32_t append(const FreeBodySupportSetCard& card, base::CardKey key);
    [[nodiscard]] FreeBodySupportSetCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeBodySupportSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
};

} // namespace oor::model
