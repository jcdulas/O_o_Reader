#pragma once

#include "oor/model/cards/loads/loadCombination/LoadCombinationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct LoadCombinationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LoadCombinationCard& card);
    [[nodiscard]] static std::optional<LoadCombinationCard> read(FieldReader& reader);
    static void write(const LoadCombinationCard& card, FieldWriter& writer);
};

// All the LOAD cards. The ID is a load set ID shared by many cards (IdSpace::Group). The terms of every card are two
// lists (scale factors, load set IDs) in CSR form.
class LoadCombinationTable final : public TypedTable<LoadCombinationTable, LoadCombinationCard>
{
public:
    static constexpr std::string_view NAME = "LoadCombination";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = LoadCombinationFormat;

    [[nodiscard]] static base::EntityId cardId(const LoadCombinationCard& card) noexcept { return card.id; }
    std::uint32_t append(const LoadCombinationCard& card, base::CardKey key);
    [[nodiscard]] LoadCombinationCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const LoadCombinationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& scaleColumn() noexcept { return m_scale; }
    [[nodiscard]] VariableLengthColumn<double>& termScaleColumn() noexcept { return m_termScale; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& termLoadColumn() noexcept { return m_termLoad; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_scale;
    VariableLengthColumn<double> m_termScale;
    VariableLengthColumn<base::EntityId> m_termLoad;
};

} // namespace oor::model
