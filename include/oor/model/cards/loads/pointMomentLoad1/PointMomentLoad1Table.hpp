#pragma once

#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PointMomentLoad1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PointMomentLoad1Card& card);
    [[nodiscard]] static std::optional<PointMomentLoad1Card> read(FieldReader& reader);
    static void write(const PointMomentLoad1Card& card, FieldWriter& writer);
};

// All the MOMENT1 cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class PointMomentLoad1Table final : public TypedTable<PointMomentLoad1Table, PointMomentLoad1Card>
{
public:
    static constexpr std::string_view NAME = "PointMomentLoad1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = PointMomentLoad1Format;

    [[nodiscard]] static base::EntityId cardId(const PointMomentLoad1Card& card) noexcept { return card.id; }
    std::uint32_t append(const PointMomentLoad1Card& card, base::CardKey key);
    [[nodiscard]] PointMomentLoad1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PointMomentLoad1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<double>& magnitudeColumn() noexcept { return m_magnitude; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<double> m_magnitude;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node2;
};

} // namespace oor::model
