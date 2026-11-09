#pragma once

#include "oor/model/cards/matrices/directMatrix/DirectMatrixCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DirectMatrixFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DirectMatrixCard& card);
    [[nodiscard]] static std::optional<DirectMatrixCard> read(FieldReader& reader);
    static void write(const DirectMatrixCard& card, FieldWriter& writer);
};

// All the DMI cards: the headers and the columns of every matrix, one row each, in the order of the deck. A card has no
// numeric ID (IdSpace::None); the matrix is found by the name. The fields of a column after its first row are three
// lists in CSR form (kind, value, row).
class DirectMatrixTable final : public TypedTable<DirectMatrixTable, DirectMatrixCard>
{
public:
    static constexpr std::string_view NAME = "DirectMatrix";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Matrix, base::IdSpace::None};
    using Format = DirectMatrixFormat;

    [[nodiscard]] static base::EntityId cardId(const DirectMatrixCard&) noexcept { return 0; }
    std::uint32_t append(const DirectMatrixCard& card, base::CardKey key);
    [[nodiscard]] DirectMatrixCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DirectMatrixCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a DMI refers to no card
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }
    [[nodiscard]] Column<std::int32_t>& columnIndexColumn() noexcept { return m_columnIndex; }
    [[nodiscard]] Column<std::int32_t>& matrixFormColumn() noexcept { return m_matrixForm; }
    [[nodiscard]] Column<std::int32_t>& inputTypeColumn() noexcept { return m_inputType; }
    [[nodiscard]] Column<std::int32_t>& outputTypeColumn() noexcept { return m_outputType; }
    [[nodiscard]] Column<std::int32_t>& rowsColumn() noexcept { return m_matrixRows; }
    [[nodiscard]] Column<std::int32_t>& columnsColumn() noexcept { return m_columns; }
    [[nodiscard]] Column<std::int32_t>& firstRowColumn() noexcept { return m_firstRow; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<double>& valueColumn() noexcept { return m_value; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& rowColumn() noexcept { return m_row; }

private:
    Column<std::string> m_entryName;
    Column<std::int32_t> m_columnIndex;
    Column<std::int32_t> m_matrixForm;
    Column<std::int32_t> m_inputType;
    Column<std::int32_t> m_outputType;
    Column<std::int32_t> m_matrixRows;
    Column<std::int32_t> m_columns;
    Column<std::int32_t> m_firstRow;
    VariableLengthColumn<std::uint8_t> m_kind;
    VariableLengthColumn<double> m_value;
    VariableLengthColumn<std::int32_t> m_row;
};

} // namespace oor::model
