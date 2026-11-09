#pragma once

#include "oor/model/cards/dynamics/complexEigenMethod/ComplexEigenMethodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace oor::model
{

struct ComplexEigenMethodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ComplexEigenMethodCard& card);
    [[nodiscard]] static std::optional<ComplexEigenMethodCard> read(FieldReader& reader);
    static void write(const ComplexEigenMethodCard& card, FieldWriter& writer);
};

// All the EIGC cards. The ID is the one the CMETHOD command selects: unique among the EIGC cards
// (IdSpace::PerCardType; METHOD selects the real methods EIGR, EIGRL and EIGB, in the method space, and the two
// commands are separate: an EIGRL and an EIGC may have the same ID), in the category
// ComplexMethod with the EIGP cards of the same ID. A blank E is NaN in its column. The members of the search regions
// are one list each, in CSR form.
class ComplexEigenMethodTable final : public TypedTable<ComplexEigenMethodTable, ComplexEigenMethodCard>
{
public:
    static constexpr std::string_view NAME = "ComplexEigenMethod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ComplexMethod, base::IdSpace::PerCardType};
    using Format = ComplexEigenMethodFormat;

    [[nodiscard]] static base::EntityId cardId(const ComplexEigenMethodCard& card) noexcept { return card.id; }
    std::uint32_t append(const ComplexEigenMethodCard& card, base::CardKey key);
    [[nodiscard]] ComplexEigenMethodCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ComplexEigenMethodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& methodNameColumn() noexcept { return m_methodName; }
    [[nodiscard]] Column<std::uint8_t>& normColumn() noexcept { return m_norm; }
    [[nodiscard]] Column<base::EntityId>& normNodeColumn() noexcept { return m_normNode; }
    [[nodiscard]] Column<std::int32_t>& normComponentColumn() noexcept { return m_normComponent; }
    [[nodiscard]] Column<double>& convergenceToleranceColumn() noexcept { return m_convergenceTolerance; }
    [[nodiscard]] Column<std::int32_t>& desiredModeCount0Column() noexcept { return m_desiredModeCount0; }
    [[nodiscard]] VariableLengthColumn<double>& alphaAColumn() noexcept { return m_alphaA; }
    [[nodiscard]] VariableLengthColumn<double>& omegaAColumn() noexcept { return m_omegaA; }
    [[nodiscard]] VariableLengthColumn<double>& alphaBColumn() noexcept { return m_alphaB; }
    [[nodiscard]] VariableLengthColumn<double>& omegaBColumn() noexcept { return m_omegaB; }
    [[nodiscard]] VariableLengthColumn<double>& regionWidthColumn() noexcept { return m_regionWidth; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& estimatedModeCountColumn() noexcept
    {
        return m_estimatedModeCount;
    }
    [[nodiscard]] VariableLengthColumn<double>& maximumBlockSizeColumn() noexcept { return m_maximumBlockSize; }
    [[nodiscard]] VariableLengthColumn<double>& initialBlockSizeColumn() noexcept { return m_initialBlockSize; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& solveIntervalColumn() noexcept { return m_solveInterval; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& desiredModeCountColumn() noexcept { return m_desiredModeCount; }

private:
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_methodName;
    Column<std::uint8_t> m_norm;
    Column<base::EntityId> m_normNode;
    Column<std::int32_t> m_normComponent;
    Column<double> m_convergenceTolerance{BLANK};
    Column<std::int32_t> m_desiredModeCount0;
    VariableLengthColumn<double> m_alphaA;
    VariableLengthColumn<double> m_omegaA;
    VariableLengthColumn<double> m_alphaB;
    VariableLengthColumn<double> m_omegaB;
    VariableLengthColumn<double> m_regionWidth;
    VariableLengthColumn<std::int32_t> m_estimatedModeCount;
    VariableLengthColumn<double> m_maximumBlockSize;
    VariableLengthColumn<double> m_initialBlockSize;
    VariableLengthColumn<std::int32_t> m_solveInterval;
    VariableLengthColumn<std::int32_t> m_desiredModeCount;
};

} // namespace oor::model
