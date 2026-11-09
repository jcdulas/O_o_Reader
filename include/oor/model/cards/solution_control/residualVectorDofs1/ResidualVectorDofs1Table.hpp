#pragma once

#include "oor/model/cards/solution_control/residualVectorDofs1/ResidualVectorDofs1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct ResidualVectorDofs1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ResidualVectorDofs1Card& card);
    [[nodiscard]] static std::optional<ResidualVectorDofs1Card> read(FieldReader& reader);
    static void write(const ResidualVectorDofs1Card& card, FieldWriter& writer);
};

// All the RVDOF1 cards. A card has no ID (IdSpace::None). The points of every card are one column of (first, last)
// runs.
class ResidualVectorDofs1Table final : public TypedTable<ResidualVectorDofs1Table, ResidualVectorDofs1Card>
{
public:
    static constexpr std::string_view NAME = "ResidualVectorDofs1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = ResidualVectorDofs1Format;

    [[nodiscard]] static base::EntityId cardId(const ResidualVectorDofs1Card&) noexcept { return 0; }
    std::uint32_t append(const ResidualVectorDofs1Card& card, base::CardKey key);
    [[nodiscard]] ResidualVectorDofs1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ResidualVectorDofs1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<std::int32_t> m_components;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
