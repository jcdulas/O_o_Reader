#pragma once

#include "oor/model/cards/rigid_elements/rigidSpline/RigidSplineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidSplineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidSplineCard& card);
    [[nodiscard]] static std::optional<RigidSplineCard> read(FieldReader& reader);
    static void write(const RigidSplineCard& card, FieldWriter& writer);
};

// All the RSPLINE cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The
// points and the components of every card are two lists in CSR form.
class RigidSplineTable final : public TypedTable<RigidSplineTable, RigidSplineCard>
{
public:
    static constexpr std::string_view NAME = "RigidSpline";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidSplineFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidSplineCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidSplineCard& card, base::CardKey key);
    [[nodiscard]] RigidSplineCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidSplineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& diameterRatioColumn() noexcept { return m_diameterRatio; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_diameterRatio{0.1};
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
};

} // namespace oor::model
