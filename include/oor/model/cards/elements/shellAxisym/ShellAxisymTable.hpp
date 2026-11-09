#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/elements/shellAxisym/ShellAxisymCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellAxisymFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellAxisymCard& card);
    [[nodiscard]] static std::optional<ShellAxisymCard> read(FieldReader& reader);
    static void write(const ShellAxisymCard& card, FieldWriter& writer);
};

// All the CAXISYM elements: an element ID, in the space every element shares (IdSpace::Element). PID names a PAXISYM
// (a property) and the points are grid points. The nodes are one column of three IDs per element (0 for a blank G3);
// the column of NOFF holds a NaN where the field is blank.
class ShellAxisymTable final : public TypedTable<ShellAxisymTable, ShellAxisymCard>
{
public:
    static constexpr std::string_view NAME = "ShellAxisym";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellAxisymFormat;

    ShellAxisymTable() : m_offset(BLANK_REAL) {}

    [[nodiscard]] static base::EntityId cardId(const ShellAxisymCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellAxisymCard& card, base::CardKey key);
    [[nodiscard]] ShellAxisymCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellAxisymCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, ShellAxisymCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& offsetColumn() noexcept { return m_offset; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, ShellAxisymCard::NODE_COUNT>> m_nodes;
    Column<double> m_offset;
};

} // namespace oor::model
