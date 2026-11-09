#pragma once

#include "oor/model/cards/matrices/directMatrixAxisym/DirectMatrixAxisymCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DirectMatrixAxisymFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DirectMatrixAxisymCard& card);
    [[nodiscard]] static std::optional<DirectMatrixAxisymCard> read(FieldReader& reader);
    static void write(const DirectMatrixAxisymCard& card, FieldWriter& writer);
};

// All the DMIAX cards: the headers and the columns of every matrix, one row each, in the order of the deck. A card has
// no numeric ID (IdSpace::None); the matrix is found by the name. The terms of a column are five lists in CSR form
// (point, component, harmonic, real part, imaginary part). The points of the column and of its terms are grid, scalar
// or extra points (the fluid points RINGFL, PRESPT and FREEPT are not typed cards yet, so a field may name one that the
// model does not have: ScalarOrGridPoint, which is never reported as missing).
class DirectMatrixAxisymTable final : public TypedTable<DirectMatrixAxisymTable, DirectMatrixAxisymCard>
{
public:
    static constexpr std::string_view NAME = "DirectMatrixAxisym";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Matrix, base::IdSpace::None};
    using Format = DirectMatrixAxisymFormat;

    DirectMatrixAxisymTable();

    [[nodiscard]] static base::EntityId cardId(const DirectMatrixAxisymCard&) noexcept { return 0; }
    std::uint32_t append(const DirectMatrixAxisymCard& card, base::CardKey key);
    [[nodiscard]] DirectMatrixAxisymCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DirectMatrixAxisymCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A column of a matrix that has no header, and a second header of one name (the format reference: only one header),
    // are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }
    [[nodiscard]] Column<base::EntityId>& nodeJColumn() noexcept { return m_nodeJ; }
    [[nodiscard]] Column<std::int32_t>& componentsJColumn() noexcept { return m_componentsJ; }
    [[nodiscard]] Column<std::int32_t>& harmonicJColumn() noexcept { return m_harmonicJ; }
    [[nodiscard]] Column<std::int32_t>& matrixShapeColumn() noexcept { return m_matrixShape; }
    [[nodiscard]] Column<std::int32_t>& inputTypeColumn() noexcept { return m_inputType; }
    [[nodiscard]] Column<std::int32_t>& outputTypeColumn() noexcept { return m_outputType; }
    [[nodiscard]] Column<std::uint8_t>& imaginaryColumn() noexcept { return m_imaginary; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& pointColumn() noexcept { return m_point; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& harmonicColumn() noexcept { return m_harmonic; }
    [[nodiscard]] VariableLengthColumn<double>& realColumn() noexcept { return m_real; }
    [[nodiscard]] VariableLengthColumn<double>& imaginaryPartColumn() noexcept { return m_imaginaryPart; }

private:
    Column<std::string> m_entryName;
    Column<base::EntityId> m_nodeJ;
    Column<std::int32_t> m_componentsJ;
    Column<std::int32_t> m_harmonicJ;
    Column<std::int32_t> m_matrixShape;
    Column<std::int32_t> m_inputType;
    Column<std::int32_t> m_outputType;
    Column<std::uint8_t> m_imaginary;
    VariableLengthColumn<base::EntityId> m_point;
    VariableLengthColumn<std::int32_t> m_component;
    VariableLengthColumn<std::int32_t> m_harmonic;
    VariableLengthColumn<double> m_real;
    VariableLengthColumn<double> m_imaginaryPart;
};

} // namespace oor::model
