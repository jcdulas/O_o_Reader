#pragma once

#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace oor::model
{

struct LanczosEigenMethodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LanczosEigenMethodCard& card);
    [[nodiscard]] static std::optional<LanczosEigenMethodCard> read(FieldReader& reader);
    static void write(const LanczosEigenMethodCard& card, FieldWriter& writer);
};

// All the EIGRL cards. The ID is a method ID, in the ID space that EIGR shares (IdSpace::Method). A blank V1, V2 or
// SHFSCL is NaN in its column; the frequencies of the segments are a list per card in CSR form.
class LanczosEigenMethodTable final : public TypedTable<LanczosEigenMethodTable, LanczosEigenMethodCard>
{
public:
    static constexpr std::string_view NAME = "LanczosEigenMethod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Method, base::IdSpace::Method};
    using Format = LanczosEigenMethodFormat;

    [[nodiscard]] static base::EntityId cardId(const LanczosEigenMethodCard& card) noexcept { return card.id; }
    std::uint32_t append(const LanczosEigenMethodCard& card, base::CardKey key);
    [[nodiscard]] LanczosEigenMethodCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const LanczosEigenMethodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an EIGRL refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& lowerFrequencyColumn() noexcept { return m_lowerFrequency; }
    [[nodiscard]] Column<double>& upperFrequencyColumn() noexcept { return m_upperFrequency; }
    [[nodiscard]] Column<std::int32_t>& desiredModeCountColumn() noexcept { return m_desiredModeCount; }
    [[nodiscard]] Column<std::int32_t>& messageLevelColumn() noexcept { return m_messageLevel; }
    [[nodiscard]] Column<std::int32_t>& maximumSetColumn() noexcept { return m_maximumSet; }
    [[nodiscard]] Column<double>& shiftScaleColumn() noexcept { return m_shiftScale; }
    [[nodiscard]] Column<std::uint8_t>& normColumn() noexcept { return m_norm; }
    [[nodiscard]] Column<double>& alphaColumn() noexcept { return m_alpha; }
    [[nodiscard]] Column<std::int32_t>& frequencySegmentCountColumn() noexcept { return m_frequencySegmentCount; }
    [[nodiscard]] VariableLengthColumn<double>& segmentFrequenciesColumn() noexcept { return m_segmentFrequencies; }

private:
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    Column<base::EntityId> m_id;
    Column<double> m_lowerFrequency{BLANK};
    Column<double> m_upperFrequency{BLANK};
    Column<std::int32_t> m_desiredModeCount;
    Column<std::int32_t> m_messageLevel;
    Column<std::int32_t> m_maximumSet;
    Column<double> m_shiftScale{BLANK};
    Column<std::uint8_t> m_norm;
    Column<double> m_alpha{1.0};
    Column<std::int32_t> m_frequencySegmentCount{1};
    VariableLengthColumn<double> m_segmentFrequencies;
};

} // namespace oor::model
