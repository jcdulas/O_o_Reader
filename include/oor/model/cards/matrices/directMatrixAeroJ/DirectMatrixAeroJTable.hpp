#pragma once

#include "oor/model/cards/matrices/directMatrixAeroJ/DirectMatrixAeroJCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DirectMatrixAeroJFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DirectMatrixAeroJCard& card);
    [[nodiscard]] static std::optional<DirectMatrixAeroJCard> read(FieldReader& reader);
    static void write(const DirectMatrixAeroJCard& card, FieldWriter& writer);
};

// All the DMIJ cards: the headers and the columns of every matrix, one row each, in the order of the deck. A card has
// no numeric ID (IdSpace::None); the matrix is found by the name. The terms of a column are four lists in CSR form
// (point, component, real part, imaginary part). The points are aerodynamic mesh points: no reference to the nodes of
// the structure is visited.
class DirectMatrixAeroJTable final : public TypedTable<DirectMatrixAeroJTable, DirectMatrixAeroJCard>
{
public:
    static constexpr std::string_view NAME = "DirectMatrixAeroJ";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Matrix, base::IdSpace::None};
    using Format = DirectMatrixAeroJFormat;

    [[nodiscard]] static base::EntityId cardId(const DirectMatrixAeroJCard&) noexcept { return 0; }
    std::uint32_t append(const DirectMatrixAeroJCard& card, base::CardKey key);
    [[nodiscard]] DirectMatrixAeroJCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DirectMatrixAeroJCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    // A column of a matrix that has no header, and a second header of one name, are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }
    [[nodiscard]] Column<base::EntityId>& nodeJColumn() noexcept { return m_nodeJ; }
    [[nodiscard]] Column<std::int32_t>& componentsJColumn() noexcept { return m_componentsJ; }
    [[nodiscard]] Column<std::int32_t>& matrixShapeColumn() noexcept { return m_matrixShape; }
    [[nodiscard]] Column<std::int32_t>& inputTypeColumn() noexcept { return m_inputType; }
    [[nodiscard]] Column<std::int32_t>& outputTypeColumn() noexcept { return m_outputType; }
    [[nodiscard]] Column<std::int32_t>& polarFlagColumn() noexcept { return m_polarFlag; }
    [[nodiscard]] Column<std::int32_t>& columnCountColumn() noexcept { return m_columnCount; }
    [[nodiscard]] Column<std::uint8_t>& imaginaryColumn() noexcept { return m_imaginary; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& pointColumn() noexcept { return m_point; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] VariableLengthColumn<double>& realColumn() noexcept { return m_real; }
    [[nodiscard]] VariableLengthColumn<double>& imaginaryPartColumn() noexcept { return m_imaginaryPart; }

private:
    Column<std::string> m_entryName;
    Column<base::EntityId> m_nodeJ;
    Column<std::int32_t> m_componentsJ;
    Column<std::int32_t> m_matrixShape;
    Column<std::int32_t> m_inputType;
    Column<std::int32_t> m_outputType;
    Column<std::int32_t> m_polarFlag;
    Column<std::int32_t> m_columnCount;
    Column<std::uint8_t> m_imaginary;
    VariableLengthColumn<base::EntityId> m_point;
    VariableLengthColumn<std::int32_t> m_component;
    VariableLengthColumn<double> m_real;
    VariableLengthColumn<double> m_imaginaryPart;
};

} // namespace oor::model
