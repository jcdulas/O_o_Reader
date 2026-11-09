#pragma once

#include "oor/model/cards/properties/beamShapeProp/BeamShapePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct BeamShapePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamShapePropCard& card);
    [[nodiscard]] static std::optional<BeamShapePropCard> read(FieldReader& reader);
    static void write(const BeamShapePropCard& card, FieldWriter& writer);

    // Gives a value to each dimension and NSM that is NaN in a station after end A, as the format says: at X/XB = 1.0
    // the value of end A, in between the linear interpolation of the two ends. Returns false when a value is blank in a
    // station but no station is at X/XB = 1.0.
    [[nodiscard]] static bool resolveBlankValues(std::vector<BeamShapePropStation>& stations);
};

// All the PBEAML cards. The stations of every card are four lists in CSR form (X/XB, output flag, NSM, dimensions): the
// row of a card gives where its stations begin and how many there are; the dimensions of all its stations follow each
// other (station after station, as many per station as the section has).
class BeamShapePropTable final : public TypedTable<BeamShapePropTable, BeamShapePropCard>
{
public:
    static constexpr std::string_view NAME = "BeamShapeProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeamShapePropFormat;

    [[nodiscard]] static base::EntityId cardId(const BeamShapePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeamShapePropCard& card, base::CardKey key);
    [[nodiscard]] BeamShapePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BeamShapePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] std::size_t stationCount(std::uint32_t row) const { return m_stationRatio.get(row).size(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<double>& stationRatioColumn() noexcept { return m_stationRatio; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& stationOutputColumn() noexcept { return m_stationOutput; }
    [[nodiscard]] VariableLengthColumn<double>& stationNonStructMassColumn() noexcept { return m_stationNonStructMass; }
    [[nodiscard]] VariableLengthColumn<double>& stationDimsColumn() noexcept { return m_stationDims; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::string> m_kind;
    VariableLengthColumn<double> m_stationRatio;
    VariableLengthColumn<std::uint8_t> m_stationOutput;
    VariableLengthColumn<double> m_stationNonStructMass;
    VariableLengthColumn<double> m_stationDims;
};

} // namespace oor::model
