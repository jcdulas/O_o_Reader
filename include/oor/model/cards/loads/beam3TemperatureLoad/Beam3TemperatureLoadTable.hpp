#pragma once

#include "oor/model/cards/loads/beam3TemperatureLoad/Beam3TemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct Beam3TemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Beam3TemperatureLoadCard& card);
    [[nodiscard]] static std::optional<Beam3TemperatureLoadCard> read(FieldReader& reader);
    static void write(const Beam3TemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMPB3 cards. The ID is a temperature set ID shared by many cards (IdSpace::Group, category Load), so that
// `TEMP = n` and a TLOAD1 EXCITEID find it. The element of the EID field is an Element reference and so is every
// element of the list; the elements of a THRU range are not looked up (ElementInRange: the two ends may not exist).
// The twelve temperatures of a card are columns of three (one value per station), the list is one CSR column of
// (first, last, by) runs.
class Beam3TemperatureLoadTable final : public TypedTable<Beam3TemperatureLoadTable, Beam3TemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "Beam3TemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = Beam3TemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const Beam3TemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const Beam3TemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] Beam3TemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const Beam3TemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }
    [[nodiscard]] Column<std::array<double, 3>>& temperatureColumn() noexcept { return m_temperature; }
    [[nodiscard]] Column<std::array<double, 3>>& gradientYColumn() noexcept { return m_gradientY; }
    [[nodiscard]] Column<std::array<double, 3>>& gradientZColumn() noexcept { return m_gradientZ; }
    [[nodiscard]] Column<std::array<double, 3>>& temperatureCColumn() noexcept { return m_temperatureC; }
    [[nodiscard]] Column<std::array<double, 3>>& temperatureDColumn() noexcept { return m_temperatureD; }
    [[nodiscard]] Column<std::array<double, 3>>& temperatureEColumn() noexcept { return m_temperatureE; }
    [[nodiscard]] Column<std::array<double, 3>>& temperatureFColumn() noexcept { return m_temperatureF; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_elemId;
    Column<std::array<double, 3>> m_temperature;
    Column<std::array<double, 3>> m_gradientY;
    Column<std::array<double, 3>> m_gradientZ;
    Column<std::array<double, 3>> m_temperatureC;
    Column<std::array<double, 3>> m_temperatureD;
    Column<std::array<double, 3>> m_temperatureE;
    Column<std::array<double, 3>> m_temperatureF;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
