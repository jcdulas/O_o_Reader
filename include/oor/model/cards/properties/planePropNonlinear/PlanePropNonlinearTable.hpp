#pragma once

#include "oor/model/cards/properties/planePropNonlinear/PlanePropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PlanePropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlanePropNonlinearCard& card);
    [[nodiscard]] static std::optional<PlanePropNonlinearCard> read(FieldReader& reader);
    static void write(const PlanePropNonlinearCard& card, FieldWriter& writer);
};

class PlanePropNonlinearTable final : public TypedTable<PlanePropNonlinearTable, PlanePropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "PlanePropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = PlanePropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const PlanePropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlanePropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] PlanePropNonlinearCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlanePropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::uint8_t>& gaussOutputColumn() noexcept { return m_gaussOutput; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<base::EntityId> m_frameId;
    Column<std::uint8_t> m_gaussOutput;
};

} // namespace oor::model
