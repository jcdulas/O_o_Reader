#pragma once

#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct LineElemTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LineElemTemperatureLoadCard& card);
    [[nodiscard]] static std::optional<LineElemTemperatureLoadCard> read(FieldReader& reader);
    static void write(const LineElemTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMPRB cards. The ID is a temperature set ID shared by many cards (IdSpace::Group, category Load). The
// element lists are one CSR column of (first, last) runs.
class LineElemTemperatureLoadTable final : public TypedTable<LineElemTemperatureLoadTable, LineElemTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "LineElemTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = LineElemTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const LineElemTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const LineElemTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] LineElemTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const LineElemTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& temperatureAColumn() noexcept { return m_temperatureA; }
    [[nodiscard]] Column<double>& temperatureBColumn() noexcept { return m_temperatureB; }
    [[nodiscard]] Column<std::array<double, 2>>& gradientYColumn() noexcept { return m_gradientY; }
    [[nodiscard]] Column<std::array<double, 2>>& gradientZColumn() noexcept { return m_gradientZ; }
    [[nodiscard]] Column<std::array<double, 4>>& pointsAColumn() noexcept { return m_pointsA; }
    [[nodiscard]] Column<std::array<double, 4>>& pointsBColumn() noexcept { return m_pointsB; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_temperatureA;
    Column<double> m_temperatureB;
    Column<std::array<double, 2>> m_gradientY;
    Column<std::array<double, 2>> m_gradientZ;
    Column<std::array<double, 4>> m_pointsA;
    Column<std::array<double, 4>> m_pointsB;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
