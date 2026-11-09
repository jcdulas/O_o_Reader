#pragma once

#include "oor/model/cards/thermal/axisymFluidSymmetry/AxisymFluidSymmetryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AxisymFluidSymmetryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidSymmetryCard& card);
    [[nodiscard]] static std::optional<AxisymFluidSymmetryCard> read(FieldReader& reader);
    static void write(const AxisymFluidSymmetryCard& card, FieldWriter& writer);
};

// All the FLSYM cards. Only one is allowed in a deck; the card has no ID (IdSpace::None, category
// Other).
class AxisymFluidSymmetryTable final : public TypedTable<AxisymFluidSymmetryTable, AxisymFluidSymmetryCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidSymmetry";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymFluidSymmetryFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidSymmetryCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymFluidSymmetryCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidSymmetryCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluidSymmetryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an FLSYM refers to no card
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::int32_t>& sectionCountColumn() noexcept { return m_sectionCount; }
    [[nodiscard]] Column<std::string>& planeSymmetry1Column() noexcept { return m_planeSymmetry1; }
    [[nodiscard]] Column<std::string>& planeSymmetry2Column() noexcept { return m_planeSymmetry2; }

private:
    Column<std::int32_t> m_sectionCount;
    Column<std::string> m_planeSymmetry1;
    Column<std::string> m_planeSymmetry2;
};

} // namespace oor::model
