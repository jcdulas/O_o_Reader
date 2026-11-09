#pragma once

#include "oor/model/cards/dynamics/transferFunction/TransferFunctionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransferFunctionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransferFunctionCard& card);
    [[nodiscard]] static std::optional<TransferFunctionCard> read(FieldReader& reader);
    static void write(const TransferFunctionCard& card, FieldWriter& writer);
};

// All the TF cards. The ID is a set ID shared by the TF cards (IdSpace::Group, category TransferFunction: the TFL
// command selects the sets). GD and the G of the terms are grid, scalar or extra points. The terms of every card are
// lists in CSR form.
class TransferFunctionTable final : public TypedTable<TransferFunctionTable, TransferFunctionCard>
{
public:
    static constexpr std::string_view NAME = "TransferFunction";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TransferFunction, base::IdSpace::Group};
    using Format = TransferFunctionFormat;

    [[nodiscard]] static base::EntityId cardId(const TransferFunctionCard& card) noexcept { return card.id; }
    std::uint32_t append(const TransferFunctionCard& card, base::CardKey key);
    [[nodiscard]] TransferFunctionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TransferFunctionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeDColumn() noexcept { return m_nodeD; }
    [[nodiscard]] Column<std::int32_t>& componentsDColumn() noexcept { return m_componentsD; }
    [[nodiscard]] Column<double>& coefficientB0Column() noexcept { return m_coefficientB0; }
    [[nodiscard]] Column<double>& coefficientB1Column() noexcept { return m_coefficientB1; }
    [[nodiscard]] Column<double>& coefficientB2Column() noexcept { return m_coefficientB2; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& termNodesColumn() noexcept { return m_termNodes; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& termComponentsColumn() noexcept { return m_termComponents; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientsA0Column() noexcept { return m_coefficientsA0; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientsA1Column() noexcept { return m_coefficientsA1; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientsA2Column() noexcept { return m_coefficientsA2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeD;
    Column<std::int32_t> m_componentsD;
    Column<double> m_coefficientB0;
    Column<double> m_coefficientB1;
    Column<double> m_coefficientB2;
    VariableLengthColumn<base::EntityId> m_termNodes;
    VariableLengthColumn<std::int32_t> m_termComponents;
    VariableLengthColumn<double> m_coefficientsA0;
    VariableLengthColumn<double> m_coefficientsA1;
    VariableLengthColumn<double> m_coefficientsA2;
};

} // namespace oor::model
