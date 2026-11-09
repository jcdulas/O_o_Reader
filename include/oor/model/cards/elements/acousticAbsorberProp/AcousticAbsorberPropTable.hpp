#pragma once

#include "oor/model/cards/elements/acousticAbsorberProp/AcousticAbsorberPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticAbsorberPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticAbsorberPropCard& card);
    [[nodiscard]] static std::optional<AcousticAbsorberPropCard> read(FieldReader& reader);
    static void write(const AcousticAbsorberPropCard& card, FieldWriter& writer);
};

// All the PAABSF cards. The ID is a property ID, in the space every property shares (IdSpace::Property). TZREID and
// TZIMID are references to dynamic tables.
class AcousticAbsorberPropTable final : public TypedTable<AcousticAbsorberPropTable, AcousticAbsorberPropCard>
{
public:
    static constexpr std::string_view NAME = "AcousticAbsorberProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AcousticAbsorberPropFormat;

    AcousticAbsorberPropTable();

    [[nodiscard]] static base::EntityId cardId(const AcousticAbsorberPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticAbsorberPropCard& card, base::CardKey key);
    [[nodiscard]] AcousticAbsorberPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticAbsorberPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& resistanceTableIdColumn() noexcept { return m_resistanceTableId; }
    [[nodiscard]] Column<base::EntityId>& reactanceTableIdColumn() noexcept { return m_reactanceTableId; }
    [[nodiscard]] Column<double>& impedanceScaleColumn() noexcept { return m_impedanceScale; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<double>& dampingColumn() noexcept { return m_damping; }
    [[nodiscard]] Column<double>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& characteristicImpedanceColumn() noexcept { return m_characteristicImpedance; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_resistanceTableId;
    Column<base::EntityId> m_reactanceTableId;
    Column<double> m_impedanceScale;
    Column<double> m_area;
    Column<double> m_damping;
    Column<double> m_stiffness;
    Column<double> m_characteristicImpedance;
};

} // namespace oor::model
