#pragma once

#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct MultiPointConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MultiPointConstrCard& card);
    [[nodiscard]] static std::optional<MultiPointConstrCard> read(FieldReader& reader);
    static void write(const MultiPointConstrCard& card, FieldWriter& writer);
};

// All the MPC cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The terms of every card are
// three lists (points, components, coefficients) in CSR form.
class MultiPointConstrTable final : public TypedTable<MultiPointConstrTable, MultiPointConstrCard>
{
public:
    static constexpr std::string_view NAME = "MultiPointConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = MultiPointConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const MultiPointConstrCard& card) noexcept { return card.id; }
    std::uint32_t append(const MultiPointConstrCard& card, base::CardKey key);
    [[nodiscard]] MultiPointConstrCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MultiPointConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientColumn() noexcept { return m_coefficient; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
    VariableLengthColumn<double> m_coefficient;
};

} // namespace oor::model
