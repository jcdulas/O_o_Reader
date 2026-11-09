#pragma once

#include "oor/model/cards/elements/generalMatrix/GeneralMatrixCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct GeneralMatrixFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GeneralMatrixCard& card);
    [[nodiscard]] static std::optional<GeneralMatrixCard> read(FieldReader& reader);
    static void write(const GeneralMatrixCard& card, FieldWriter& writer);
};

// All the GENEL elements. The degree-of-freedom lists are (point, component) pairs, the matrices lists of reals.
class GeneralMatrixTable final : public TypedTable<GeneralMatrixTable, GeneralMatrixCard>
{
public:
    static constexpr std::string_view NAME = "GeneralMatrix";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = GeneralMatrixFormat;

    [[nodiscard]] static base::EntityId cardId(const GeneralMatrixCard& card) noexcept { return card.id; }
    std::uint32_t append(const GeneralMatrixCard& card, base::CardKey key);
    [[nodiscard]] GeneralMatrixCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GeneralMatrixCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& independentDofsColumn() noexcept
    {
        return m_independentDofs;
    }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& dependentDofsColumn() noexcept
    {
        return m_dependentDofs;
    }
    [[nodiscard]] Column<std::uint8_t>& flexibilityColumn() noexcept { return m_flexibility; }
    [[nodiscard]] VariableLengthColumn<double>& stiffnessMatrixColumn() noexcept { return m_stiffnessMatrix; }
    [[nodiscard]] VariableLengthColumn<double>& dependencyMatrixColumn() noexcept { return m_dependencyMatrix; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_independentDofs;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_dependentDofs;
    Column<std::uint8_t> m_flexibility;
    VariableLengthColumn<double> m_stiffnessMatrix;
    VariableLengthColumn<double> m_dependencyMatrix;
};

} // namespace oor::model
