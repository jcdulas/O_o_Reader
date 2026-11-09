#pragma once

#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct NonStructMassLumpedFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonStructMassLumpedCard& card);
    [[nodiscard]] static std::optional<NonStructMassLumpedCard> read(FieldReader& reader);
    static void write(const NonStructMassLumpedCard& card, FieldWriter& writer);
};

// All the NSML cards. The ID is a set ID shared by many cards (IdSpace::Group); the IDs and the values of every card
// are two lists in CSR form.
class NonStructMassLumpedTable final : public TypedTable<NonStructMassLumpedTable, NonStructMassLumpedCard>
{
public:
    static constexpr std::string_view NAME = "NonStructMassLumped";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = NonStructMassLumpedFormat;

    [[nodiscard]] static base::EntityId cardId(const NonStructMassLumpedCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonStructMassLumpedCard& card, base::CardKey key);
    [[nodiscard]] NonStructMassLumpedCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonStructMassLumpedCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& targetIdsColumn() noexcept { return m_ids; }
    [[nodiscard]] VariableLengthColumn<double>& targetValuesColumn() noexcept { return m_values; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_type;
    VariableLengthColumn<base::EntityId> m_ids;
    VariableLengthColumn<double> m_values;
};

} // namespace oor::model
