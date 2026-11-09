#pragma once

#include "oor/model/cards/elements/bar2Stations/Bar2StationsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct Bar2StationsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Bar2StationsCard& card);
    [[nodiscard]] static std::optional<Bar2StationsCard> read(FieldReader& reader);
    static void write(const Bar2StationsCard& card, FieldWriter& writer);
};

// All the CBARAO cards. The ID is the one of the CBAR the stations belong to: it is not an element ID of its own, so
// the card has its own ID space (one CBARAO per bar) and refers to an element.
class Bar2StationsTable final : public TypedTable<Bar2StationsTable, Bar2StationsCard>
{
public:
    static constexpr std::string_view NAME = "Bar2Stations";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = Bar2StationsFormat;

    Bar2StationsTable() : m_stationScale(std::string("FractionalLength")) {}

    [[nodiscard]] static base::EntityId cardId(const Bar2StationsCard& card) noexcept { return card.id; }
    std::uint32_t append(const Bar2StationsCard& card, base::CardKey key);
    [[nodiscard]] Bar2StationsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const Bar2StationsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& stationScaleColumn() noexcept { return m_stationScale; }
    [[nodiscard]] VariableLengthColumn<double>& listedStationsColumn() noexcept { return m_listedStations; }
    [[nodiscard]] Column<std::int32_t>& stationCountColumn() noexcept { return m_stationCount; }
    [[nodiscard]] Column<double>& firstColumn() noexcept { return m_first; }
    [[nodiscard]] Column<double>& stationSpacingColumn() noexcept { return m_stationSpacing; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_stationScale;
    VariableLengthColumn<double> m_listedStations;
    Column<std::int32_t> m_stationCount;
    Column<double> m_first;
    Column<double> m_stationSpacing;
};

} // namespace oor::model
