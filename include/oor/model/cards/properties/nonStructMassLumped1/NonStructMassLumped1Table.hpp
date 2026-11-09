#pragma once

#include "oor/model/cards/properties/nonStructMassLumped1/NonStructMassLumped1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct NonStructMassLumped1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonStructMassLumped1Card& card);
    [[nodiscard]] static std::optional<NonStructMassLumped1Card> read(FieldReader& reader);
    static void write(const NonStructMassLumped1Card& card, FieldWriter& writer);
};

// All the NSML1 cards. The ID is a set ID shared by many cards (IdSpace::Group). The list of IDs of every card is one
// CSR column of (first, last, by) runs, and a flag for ALL.
class NonStructMassLumped1Table final : public TypedTable<NonStructMassLumped1Table, NonStructMassLumped1Card>
{
public:
    static constexpr std::string_view NAME = "NonStructMassLumped1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = NonStructMassLumped1Format;

    [[nodiscard]] static base::EntityId cardId(const NonStructMassLumped1Card& card) noexcept { return card.id; }
    std::uint32_t append(const NonStructMassLumped1Card& card, base::CardKey key);
    [[nodiscard]] NonStructMassLumped1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonStructMassLumped1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<std::uint8_t>& allColumn() noexcept { return m_all; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_type;
    Column<double> m_nonStructMass;
    Column<std::uint8_t> m_all;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
