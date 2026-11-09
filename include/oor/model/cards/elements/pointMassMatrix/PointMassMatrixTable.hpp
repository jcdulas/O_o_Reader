#pragma once

#include "oor/model/cards/elements/pointMassMatrix/PointMassMatrixCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PointMassMatrixFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PointMassMatrixCard& card);
    [[nodiscard]] static std::optional<PointMassMatrixCard> read(FieldReader& reader);
    static void write(const PointMassMatrixCard& card, FieldWriter& writer);
};

// All the CONM1 elements. The mass values are an (N, 21) NumPy view, without copy.
class PointMassMatrixTable final : public TypedTable<PointMassMatrixTable, PointMassMatrixCard>
{
public:
    static constexpr std::string_view NAME = "PointMassMatrix";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = PointMassMatrixFormat;

    PointMassMatrixTable() : m_frameId(0) {}

    [[nodiscard]] static base::EntityId cardId(const PointMassMatrixCard& card) noexcept { return card.id; }
    std::uint32_t append(const PointMassMatrixCard& card, base::CardKey key);
    [[nodiscard]] PointMassMatrixCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PointMassMatrixCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::array<double, PointMassMatrixCard::VALUE_COUNT>>& massMatrixColumn() noexcept
    {
        return m_massMatrix;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<base::EntityId> m_frameId;
    Column<std::array<double, PointMassMatrixCard::VALUE_COUNT>> m_massMatrix;
};

} // namespace oor::model
