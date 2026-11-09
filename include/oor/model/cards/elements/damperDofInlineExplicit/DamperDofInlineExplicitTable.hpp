#pragma once

#include "oor/model/cards/elements/damperDofInlineExplicit/DamperDofInlineExplicitCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DamperDofInlineExplicitFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DamperDofInlineExplicitCard& card);
    [[nodiscard]] static std::optional<DamperDofInlineExplicitCard> read(FieldReader& reader);
    static void write(const DamperDofInlineExplicitCard& card, FieldWriter& writer);
};

// All the CDAMP2D elements. They are elements (IdSpace::Element, shared with every other element), kept in columns.
class DamperDofInlineExplicitTable final : public TypedTable<DamperDofInlineExplicitTable, DamperDofInlineExplicitCard>
{
public:
    static constexpr std::string_view NAME = "DamperDofInlineExplicit";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = DamperDofInlineExplicitFormat;

    [[nodiscard]] static base::EntityId cardId(const DamperDofInlineExplicitCard& card) noexcept { return card.id; }
    std::uint32_t append(const DamperDofInlineExplicitCard& card, base::CardKey key);
    [[nodiscard]] DamperDofInlineExplicitCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DamperDofInlineExplicitCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& dampingColumn() noexcept { return m_damping; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<std::int32_t>& components1Column() noexcept { return m_components1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }
    [[nodiscard]] Column<std::int32_t>& components2Column() noexcept { return m_components2; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::uint8_t>& followColumn() noexcept { return m_follow; } // a ScalarFollow

private:
    Column<base::EntityId> m_id;
    Column<double> m_damping;
    Column<base::EntityId> m_node1;
    Column<std::int32_t> m_components1;
    Column<base::EntityId> m_node2;
    Column<std::int32_t> m_components2;
    Column<base::EntityId> m_frameId;
    Column<std::uint8_t> m_follow;
};

} // namespace oor::model
