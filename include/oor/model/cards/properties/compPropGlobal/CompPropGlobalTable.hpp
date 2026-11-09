#pragma once

#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompPropGlobalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompPropGlobalCard& card);
    [[nodiscard]] static std::optional<CompPropGlobalCard> read(FieldReader& reader);
    static void write(const CompPropGlobalCard& card, FieldWriter& writer);
};

// All the PCOMPG cards. The plies of every laminate are five lists (global IDs, materials, thicknesses, angles, output
// flags) in CSR form: the row of a laminate gives where its plies begin and how many there are.
class CompPropGlobalTable final : public TypedTable<CompPropGlobalTable, CompPropGlobalCard>
{
public:
    static constexpr std::string_view NAME = "CompPropGlobal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CompPropGlobalFormat;

    [[nodiscard]] static base::EntityId cardId(const CompPropGlobalCard& card) noexcept { return card.id; }
    std::uint32_t append(const CompPropGlobalCard& card, base::CardKey key);
    [[nodiscard]] CompPropGlobalCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CompPropGlobalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] std::size_t plyCount(std::uint32_t row) const { return m_plyGlobalId.get(row).size(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& bottomOffsetColumn() noexcept { return m_bottomOffset; }
    [[nodiscard]] Column<std::uint8_t>& bottomOffsetGivenColumn() noexcept { return m_z0Given; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& bondShearAllowableColumn() noexcept { return m_bondShearAllowable; }
    [[nodiscard]] Column<std::uint8_t>& failureTheoryColumn() noexcept { return m_failureTheory; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<std::uint8_t>& optionColumn() noexcept { return m_option; }
    [[nodiscard]] Column<std::int32_t>& structuralDampingFlagColumn() noexcept { return m_structuralDampingFlag; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& plyGlobalIdColumn() noexcept { return m_plyGlobalId; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& plyMatIdColumn() noexcept { return m_plyMatId; }
    [[nodiscard]] VariableLengthColumn<double>& plyThicknessColumn() noexcept { return m_plyThickness; }
    [[nodiscard]] VariableLengthColumn<double>& plyAngleColumn() noexcept { return m_plyAngle; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& plyStressOutputColumn() noexcept { return m_plyStressOutput; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_bottomOffset;
    Column<std::uint8_t> m_z0Given;
    Column<double> m_nonStructMass;
    Column<double> m_bondShearAllowable;
    Column<std::uint8_t> m_failureTheory;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<std::uint8_t> m_option;
    Column<std::int32_t> m_structuralDampingFlag;
    VariableLengthColumn<base::EntityId> m_plyGlobalId;
    VariableLengthColumn<base::EntityId> m_plyMatId;
    VariableLengthColumn<double> m_plyThickness;
    VariableLengthColumn<double> m_plyAngle;
    VariableLengthColumn<std::uint8_t> m_plyStressOutput;
};

} // namespace oor::model
