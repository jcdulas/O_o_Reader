#pragma once

#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ScalarPointLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ScalarPointLoadCard& card);
    [[nodiscard]] static std::optional<ScalarPointLoadCard> read(FieldReader& reader);
    static void write(const ScalarPointLoadCard& card, FieldWriter& writer);
};

// All the SLOAD cards. The ID is a load set ID shared by many cards (IdSpace::Group). The points and their loads are
// two lists in CSR form.
class ScalarPointLoadTable final : public TypedTable<ScalarPointLoadTable, ScalarPointLoadCard>
{
public:
    static constexpr std::string_view NAME = "ScalarPointLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = ScalarPointLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const ScalarPointLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const ScalarPointLoadCard& card, base::CardKey key);
    [[nodiscard]] ScalarPointLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ScalarPointLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<double>& magnitudeColumn() noexcept { return m_magnitude; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<double> m_magnitude;
};

} // namespace oor::model
