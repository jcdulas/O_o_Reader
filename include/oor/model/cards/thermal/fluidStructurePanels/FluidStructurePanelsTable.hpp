#pragma once

#include "oor/model/cards/thermal/fluidStructurePanels/FluidStructurePanelsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FluidStructurePanelsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidStructurePanelsCard& card);
    [[nodiscard]] static std::optional<FluidStructurePanelsCard> read(FieldReader& reader);
    static void write(const FluidStructurePanelsCard& card, FieldWriter& writer);
};

// All the PANEL cards. A card has no ID (IdSpace::None, category Other); the set of each panel is a reference to a SET1
// or SET3 card. The names of the panels have no column for the query engine (text lists); the sets do.
class FluidStructurePanelsTable final : public TypedTable<FluidStructurePanelsTable, FluidStructurePanelsCard>
{
public:
    static constexpr std::string_view NAME = "FluidStructurePanels";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FluidStructurePanelsFormat;

    [[nodiscard]] static base::EntityId cardId(const FluidStructurePanelsCard&) noexcept { return 0; }
    std::uint32_t append(const FluidStructurePanelsCard& card, base::CardKey key);
    [[nodiscard]] FluidStructurePanelsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FluidStructurePanelsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<std::string>& namesColumn() noexcept { return m_names; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& setsColumn() noexcept { return m_sets; }

private:
    VariableLengthColumn<std::string> m_names;
    VariableLengthColumn<base::EntityId> m_sets;
};

} // namespace oor::model
