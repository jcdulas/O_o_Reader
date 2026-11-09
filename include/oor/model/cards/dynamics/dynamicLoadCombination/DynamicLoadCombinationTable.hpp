#pragma once

#include "oor/model/cards/dynamics/dynamicLoadCombination/DynamicLoadCombinationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DynamicLoadCombinationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DynamicLoadCombinationCard& card);
    [[nodiscard]] static std::optional<DynamicLoadCombinationCard> read(FieldReader& reader);
    static void write(const DynamicLoadCombinationCard& card, FieldWriter& writer);
};

// All the DLOAD cards. The ID is a dynamic load set ID shared by the RLOAD, TLOAD and DAREA cards (IdSpace::Group). The
// terms of every card are two lists (scaleFactor factors, dynamic load set IDs) in CSR form.
class DynamicLoadCombinationTable final : public TypedTable<DynamicLoadCombinationTable, DynamicLoadCombinationCard>
{
public:
    static constexpr std::string_view NAME = "DynamicLoadCombination";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = DynamicLoadCombinationFormat;

    [[nodiscard]] static base::EntityId cardId(const DynamicLoadCombinationCard& card) noexcept { return card.id; }
    std::uint32_t append(const DynamicLoadCombinationCard& card, base::CardKey key);
    [[nodiscard]] DynamicLoadCombinationCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DynamicLoadCombinationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] VariableLengthColumn<double>& termScaleFactorsColumn() noexcept { return m_termScaleFactors; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& termLoadSetIdsColumn() noexcept { return m_termLoadSetIds; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_scaleFactor;
    VariableLengthColumn<double> m_termScaleFactors;
    VariableLengthColumn<base::EntityId> m_termLoadSetIds;
};

} // namespace oor::model
