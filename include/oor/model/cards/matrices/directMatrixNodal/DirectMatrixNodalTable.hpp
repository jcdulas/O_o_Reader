#pragma once

#include "oor/model/cards/matrices/directMatrixNodal/DirectMatrixNodalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace oor::model
{

struct DirectMatrixNodalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DirectMatrixNodalCard& card);
    [[nodiscard]] static std::optional<DirectMatrixNodalCard> read(FieldReader& reader);
    static void write(const DirectMatrixNodalCard& card, FieldWriter& writer);
};

// All the DMIG cards: the headers and the columns of every matrix, one row each, in the order of the deck. A card has
// no numeric ID (IdSpace::None); the matrix is found by the name. The terms of a column are four lists in CSR form
// (point, component, real part, imaginary part); a header and a column of a real matrix have no imaginary parts.
class DirectMatrixNodalTable final : public TypedTable<DirectMatrixNodalTable, DirectMatrixNodalCard>
{
public:
    static constexpr std::string_view NAME = "DirectMatrixNodal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Matrix, base::IdSpace::None};
    using Format = DirectMatrixNodalFormat;

    [[nodiscard]] static base::EntityId cardId(const DirectMatrixNodalCard&) noexcept { return 0; }
    std::uint32_t append(const DirectMatrixNodalCard& card, base::CardKey key);
    [[nodiscard]] DirectMatrixNodalCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DirectMatrixNodalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A header or a column that carries the name of no header is reported (a column of a matrix that has no header, a
    // second header of one name).
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
    // True when the matrix `name` has its columns numbered (IFO 9 with NCOL): GJ of a column is then a column index,
    // not a point, and a renumbering of the points leaves it alone.
    [[nodiscard]] bool columnsAreNumbered(const std::string& name);

    std::optional<std::set<std::string>> m_numbered; // found on first use, forgotten by every change of the cards
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
