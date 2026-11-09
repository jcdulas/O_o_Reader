#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/nodes/acousticSlotControl/AcousticSlotControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSlotControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSlotControlCard& card);
    [[nodiscard]] static std::optional<AcousticSlotControlCard> read(FieldReader& reader);
    static void write(const AcousticSlotControlCard& card, FieldWriter& writer);
};

// All the AXSLOT cards. Only one is allowed in a deck; the card has no ID (IdSpace::None, category
// Other). A blank real is a NaN in its column and a blank integer is NO_VALUE.
class AcousticSlotControlTable final : public TypedTable<AcousticSlotControlTable, AcousticSlotControlCard>
{
public:
    static constexpr std::string_view NAME = "AcousticSlotControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AcousticSlotControlFormat;
    static constexpr std::int32_t NO_VALUE = -1; // the column's blank N or MD (both are zero or more)

    AcousticSlotControlTable()
        : m_defaultDensity(BLANK_REAL),
          m_defaultBulkModulus(BLANK_REAL),
          m_harmonic(NO_VALUE),
          m_defaultSlotWidth(BLANK_REAL),
          m_defaultSlotCount(NO_VALUE)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const AcousticSlotControlCard&) noexcept { return 0; }
    std::uint32_t append(const AcousticSlotControlCard& card, base::CardKey key);
    [[nodiscard]] AcousticSlotControlCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticSlotControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an AXSLOT refers to no card
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<double>& defaultDensityColumn() noexcept { return m_defaultDensity; }
    [[nodiscard]] Column<double>& defaultBulkModulusColumn() noexcept { return m_defaultBulkModulus; }
    [[nodiscard]] Column<std::int32_t>& harmonicColumn() noexcept { return m_harmonic; }
    [[nodiscard]] Column<double>& defaultSlotWidthColumn() noexcept { return m_defaultSlotWidth; }
    [[nodiscard]] Column<std::int32_t>& defaultSlotCountColumn() noexcept { return m_defaultSlotCount; }

private:
    Column<double> m_defaultDensity;
    Column<double> m_defaultBulkModulus;
    Column<std::int32_t> m_harmonic;
    Column<double> m_defaultSlotWidth;
    Column<std::int32_t> m_defaultSlotCount;
};

} // namespace oor::model
