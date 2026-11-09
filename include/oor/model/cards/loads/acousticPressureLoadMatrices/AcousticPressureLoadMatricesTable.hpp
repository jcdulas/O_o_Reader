#pragma once

#include "oor/model/cards/loads/acousticPressureLoadMatrices/AcousticPressureLoadMatricesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticPressureLoadMatricesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticPressureLoadMatricesCard& card);
    [[nodiscard]] static std::optional<AcousticPressureLoadMatricesCard> read(FieldReader& reader);
    static void write(const AcousticPressureLoadMatricesCard& card, FieldWriter& writer);
};

// All the ACLOAD cards. The ID is a dynamic load set ID shared by RLOAD1, RLOAD2, ACSRCE and ACLOAD (the
// IdSpace::Group of the category DynamicLoad), so a DLOAD and the Case Control DLOAD find it. An ACLOAD names no other
// card: its units are Fortran units.
class AcousticPressureLoadMatricesTable final
    : public TypedTable<AcousticPressureLoadMatricesTable, AcousticPressureLoadMatricesCard>
{
public:
    static constexpr std::string_view NAME = "AcousticPressureLoadMatrices";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = AcousticPressureLoadMatricesFormat;

    AcousticPressureLoadMatricesTable();

    [[nodiscard]] static base::EntityId cardId(const AcousticPressureLoadMatricesCard& card) noexcept
    {
        return card.id;
    }
    std::uint32_t append(const AcousticPressureLoadMatricesCard& card, base::CardKey key);
    [[nodiscard]] AcousticPressureLoadMatricesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticPressureLoadMatricesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the units are not cards

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& dataFileUnitColumn() noexcept { return m_dataFileUnit; }
    [[nodiscard]] Column<base::EntityId>& propertyFileUnitColumn() noexcept { return m_propertyFileUnit; }
    [[nodiscard]] Column<double>& scaleRealColumn() noexcept { return m_scaleReal; }
    [[nodiscard]] Column<double>& scaleImaginaryColumn() noexcept { return m_scaleImaginary; }
    [[nodiscard]] Column<base::EntityId>& fileLoadCaseIdColumn() noexcept { return m_fileLoadCaseId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_dataFileUnit;
    Column<base::EntityId> m_propertyFileUnit;
    Column<double> m_scaleReal;
    Column<double> m_scaleImaginary;
    Column<base::EntityId> m_fileLoadCaseId;
};

} // namespace oor::model
