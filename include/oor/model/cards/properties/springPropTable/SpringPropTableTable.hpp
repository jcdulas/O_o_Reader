#pragma once

#include "oor/model/cards/properties/springPropTable/SpringPropTableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SpringPropTableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpringPropTableCard& card);
    [[nodiscard]] static std::optional<SpringPropTableCard> read(FieldReader& reader);
    static void write(const SpringPropTableCard& card, FieldWriter& writer);
};

// All the PELAS1 properties, kept in columns. The ID space is the one of every property (IdSpace::Property): a PELAS
// and a PELAS1 cannot share an ID, as the format reference says.
class SpringPropTableTable final : public TypedTable<SpringPropTableTable, SpringPropTableCard>
{
public:
    static constexpr std::string_view NAME = "SpringPropTable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = SpringPropTableFormat;

    [[nodiscard]] static base::EntityId cardId(const SpringPropTableCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpringPropTableCard& card, base::CardKey key);
    [[nodiscard]] SpringPropTableCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SpringPropTableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_tableId;
};

} // namespace oor::model
