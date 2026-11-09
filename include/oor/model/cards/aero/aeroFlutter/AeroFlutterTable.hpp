#pragma once

#include "oor/model/cards/aero/aeroFlutter/AeroFlutterCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroFlutterFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroFlutterCard& card);
    [[nodiscard]] static std::optional<AeroFlutterCard> read(FieldReader& reader);
    static void write(const AeroFlutterCard& card, FieldWriter& writer);
};

// All the FLUTTER cards, in the category AeroFlutterSet. The SID is unique among the FLUTTER cards
// (IdSpace::PerCardType) and is what the Case Control FMETHOD names (ReferenceTarget::AeroFlutterSet). DENS, MACH and
// RFREQ are FLFACT cards (ReferenceTarget::AeroFlutterFactor). The method and the interpolation are kept as the number
// of their word; a blank OMAX is a NaN in its column.
class AeroFlutterTable final : public TypedTable<AeroFlutterTable, AeroFlutterCard>
{
public:
    static constexpr std::string_view NAME = "AeroFlutter";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroFlutterSet, base::IdSpace::PerCardType};
    using Format = AeroFlutterFormat;

    AeroFlutterTable();

    [[nodiscard]] static base::EntityId cardId(const AeroFlutterCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroFlutterCard& card, base::CardKey key);
    [[nodiscard]] AeroFlutterCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroFlutterCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<base::EntityId>& densityFactorsIdColumn() noexcept { return m_densityFactorsId; }
    [[nodiscard]] Column<base::EntityId>& machFactorsIdColumn() noexcept { return m_machFactorsId; }
    [[nodiscard]] Column<base::EntityId>& reducedFreqFactorsIdColumn() noexcept { return m_reducedFreqFactorsId; }
    [[nodiscard]] Column<std::uint8_t>& interpolationMethodColumn() noexcept { return m_interpolationMethod; }
    [[nodiscard]] Column<std::int32_t>& eigenvalueCountColumn() noexcept { return m_eigenvalueCount; }
    [[nodiscard]] Column<double>& maxFrequencyColumn() noexcept { return m_maxFrequency; }
    [[nodiscard]] Column<double>& epsilonColumn() noexcept { return m_epsilon; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_method;
    Column<base::EntityId> m_densityFactorsId;
    Column<base::EntityId> m_machFactorsId;
    Column<base::EntityId> m_reducedFreqFactorsId;
    Column<std::uint8_t> m_interpolationMethod;
    Column<std::int32_t> m_eigenvalueCount;
    Column<double> m_maxFrequency;
    Column<double> m_epsilon;
};

} // namespace oor::model
