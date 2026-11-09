#pragma once

#include "oor/model/cards/loads/axialDeformationLoad/AxialDeformationLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxialDeformationLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxialDeformationLoadCard& card);
    [[nodiscard]] static std::optional<AxialDeformationLoadCard> read(FieldReader& reader);
    static void write(const AxialDeformationLoadCard& card, FieldWriter& writer);
};

// All the DEFORM cards. The ID is a deformation set ID shared by many cards (IdSpace::Group, category Load: the set is
// selected like a load set). The elements and their deformations are two lists in CSR form.
class AxialDeformationLoadTable final : public TypedTable<AxialDeformationLoadTable, AxialDeformationLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxialDeformationLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxialDeformationLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AxialDeformationLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxialDeformationLoadCard& card, base::CardKey key);
    [[nodiscard]] AxialDeformationLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxialDeformationLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& elementColumn() noexcept { return m_element; }
    [[nodiscard]] VariableLengthColumn<double>& deformationColumn() noexcept { return m_deformation; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_element;
    VariableLengthColumn<double> m_deformation;
};

} // namespace oor::model
