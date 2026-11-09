#pragma once

#include "oor/model/cards/dynamics/transientInitialCondition/TransientInitialConditionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransientInitialConditionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransientInitialConditionCard& card);
    [[nodiscard]] static std::optional<TransientInitialConditionCard> read(FieldReader& reader);
    static void write(const TransientInitialConditionCard& card, FieldWriter& writer);
};

// All the TIC cards. The ID is a set ID shared by the TIC cards (IdSpace::Group, category InitialCondition: the IC
// command selects the set). G is a grid, scalar or extra point (ScalarOrGridPoint), except for C = -1 where it is a
// modal coordinate: no reference.
class TransientInitialConditionTable final
    : public TypedTable<TransientInitialConditionTable, TransientInitialConditionCard>
{
public:
    static constexpr std::string_view NAME = "TransientInitialCondition";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialCondition, base::IdSpace::Group};
    using Format = TransientInitialConditionFormat;

    [[nodiscard]] static base::EntityId cardId(const TransientInitialConditionCard& card) noexcept { return card.id; }
    std::uint32_t append(const TransientInitialConditionCard& card, base::CardKey key);
    [[nodiscard]] TransientInitialConditionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TransientInitialConditionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<double>& initialDisplacementColumn() noexcept { return m_initialDisplacement; }
    [[nodiscard]] Column<double>& initialVelocityColumn() noexcept { return m_initialVelocity; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<std::int32_t> m_components;
    Column<double> m_initialDisplacement;
    Column<double> m_initialVelocity;
};

} // namespace oor::model
