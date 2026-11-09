#pragma once

#include "oor/model/cards/thermal/acousticAbsorberBrickProp/AcousticAbsorberBrickPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AcousticAbsorberBrickPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticAbsorberBrickPropCard& card);
    [[nodiscard]] static std::optional<AcousticAbsorberBrickPropCard> read(FieldReader& reader);
    static void write(const AcousticAbsorberBrickPropCard& card, FieldWriter& writer);
};

// All the PACABS cards. The ID is a property ID, in the space every property shares (IdSpace::Property: a CHACAB names
// it as its PID). The tables are references to dynamic tables. The absent values are NaN.
class AcousticAbsorberBrickPropTable final
    : public TypedTable<AcousticAbsorberBrickPropTable, AcousticAbsorberBrickPropCard>
{
public:
    static constexpr std::string_view NAME = "AcousticAbsorberBrickProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AcousticAbsorberBrickPropFormat;

    AcousticAbsorberBrickPropTable();

    [[nodiscard]] static base::EntityId cardId(const AcousticAbsorberBrickPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticAbsorberBrickPropCard& card, base::CardKey key);
    [[nodiscard]] AcousticAbsorberBrickPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticAbsorberBrickPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& synthesizeColumn() noexcept { return m_synthesize; }
    [[nodiscard]] Column<base::EntityId>& resistanceTableIdColumn() noexcept { return m_resistanceTableId; }
    [[nodiscard]] Column<base::EntityId>& reactanceTableIdColumn() noexcept { return m_reactanceTableId; }
    [[nodiscard]] Column<base::EntityId>& weightingTableIdColumn() noexcept { return m_weightingTableId; }
    [[nodiscard]] Column<double>& reservedValueColumn() noexcept { return m_reservedValue; }
    [[nodiscard]] Column<double>& cutoffFrequencyColumn() noexcept { return m_cutoffFrequency; }
    [[nodiscard]] Column<double>& dampingColumn() noexcept { return m_damping; }
    [[nodiscard]] Column<double>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_synthesize;
    Column<base::EntityId> m_resistanceTableId;
    Column<base::EntityId> m_reactanceTableId;
    Column<base::EntityId> m_weightingTableId;
    Column<double> m_reservedValue;
    Column<double> m_cutoffFrequency;
    Column<double> m_damping;
    Column<double> m_stiffness;
    Column<double> m_mass;
};

} // namespace oor::model
