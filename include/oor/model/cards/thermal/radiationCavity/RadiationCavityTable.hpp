#pragma once

#include "oor/model/cards/thermal/radiationCavity/RadiationCavityCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct RadiationCavityFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationCavityCard& card);
    [[nodiscard]] static std::optional<RadiationCavityCard> read(FieldReader& reader);
    static void write(const RadiationCavityCard& card, FieldWriter& writer);
};

// All the RADCAV cards. The ID is the ICAVITY that the RADLST, RADMTX, VIEW3D and VIEW cards of the cavity name
// (category RadiationCavity, IdSpace::PerCardType: one RADCAV per cavity). ELEAMB is a surface element, the SET pairs
// are SET1 cards. A blank PRTPCH is -1 in the column.
class RadiationCavityTable final : public TypedTable<RadiationCavityTable, RadiationCavityCard>
{
public:
    static constexpr std::string_view NAME = "RadiationCavity";
    static inline const CardTypeInfo INFO{NAME, CardCategory::RadiationCavity, base::IdSpace::PerCardType};
    using Format = RadiationCavityFormat;

    // The value of the PRTPCH column for a blank field.
    static constexpr std::int32_t NO_VIEW_FACTOR_OUTPUT = -1;

    RadiationCavityTable();

    [[nodiscard]] static base::EntityId cardId(const RadiationCavityCard& card) noexcept { return card.id; }
    std::uint32_t append(const RadiationCavityCard& card, base::CardKey key);
    [[nodiscard]] RadiationCavityCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationCavityCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ambientElemIdColumn() noexcept { return m_ambientElemId; }
    [[nodiscard]] Column<std::string>& shadowingColumn() noexcept { return m_shadowing; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<std::int32_t>& viewFactorOutputColumn() noexcept { return m_viewFactorOutput; }
    [[nodiscard]] Column<std::string>& viewFactorMethodColumn() noexcept { return m_viewFactorMethod; }
    [[nodiscard]] Column<double>& subdivisionAreaRatioColumn() noexcept { return m_subdivisionAreaRatio; }
    [[nodiscard]] Column<std::int32_t>& componentCountColumn() noexcept { return m_componentCount; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& setsColumn() noexcept { return m_sets; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ambientElemId;
    Column<std::string> m_shadowing;
    Column<double> m_scaleFactor;
    Column<std::int32_t> m_viewFactorOutput;
    Column<std::string> m_viewFactorMethod;
    Column<double> m_subdivisionAreaRatio;
    Column<std::int32_t> m_componentCount;
    VariableLengthColumn<base::EntityId> m_sets;
};

} // namespace oor::model
