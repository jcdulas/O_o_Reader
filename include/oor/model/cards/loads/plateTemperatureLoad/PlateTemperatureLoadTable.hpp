#pragma once

#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlateTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlateTemperatureLoadCard& card);
    [[nodiscard]] static std::optional<PlateTemperatureLoadCard> read(FieldReader& reader);
    static void write(const PlateTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMPP1 cards. The ID is a temperature set ID shared by many cards (IdSpace::Group, category Load). The
// element lists are one CSR column of (first, last) runs.
class PlateTemperatureLoadTable final : public TypedTable<PlateTemperatureLoadTable, PlateTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "PlateTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = PlateTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const PlateTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlateTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] PlateTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlateTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& meanTemperatureColumn() noexcept { return m_meanTemperature; }
    [[nodiscard]] Column<double>& temperatureGradientColumn() noexcept { return m_temperatureGradient; }
    [[nodiscard]] Column<double>& temperature1Column() noexcept { return m_temperature1; }
    [[nodiscard]] Column<double>& temperature2Column() noexcept { return m_temperature2; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_meanTemperature;
    Column<double> m_temperatureGradient;
    Column<double> m_temperature1;
    Column<double> m_temperature2;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
