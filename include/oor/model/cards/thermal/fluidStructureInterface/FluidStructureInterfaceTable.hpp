#pragma once

#include "oor/model/cards/thermal/fluidStructureInterface/FluidStructureInterfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FluidStructureInterfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidStructureInterfaceCard& card);
    [[nodiscard]] static std::optional<FluidStructureInterfaceCard> read(FieldReader& reader);
    static void write(const FluidStructureInterfaceCard& card, FieldWriter& writer);
};

// All the ACMODL cards. Only one is allowed in a deck; the card has no ID (IdSpace::None, category
// Other). FSET and SSET refer to SET1 cards.
class FluidStructureInterfaceTable final : public TypedTable<FluidStructureInterfaceTable, FluidStructureInterfaceCard>
{
public:
    static constexpr std::string_view NAME = "FluidStructureInterface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FluidStructureInterfaceFormat;

    FluidStructureInterfaceTable();

    [[nodiscard]] static base::EntityId cardId(const FluidStructureInterfaceCard&) noexcept { return 0; }
    std::uint32_t append(const FluidStructureInterfaceCard& card, base::CardKey key);
    [[nodiscard]] FluidStructureInterfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FluidStructureInterfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& interfaceKindColumn() noexcept { return m_interfaceKind; }
    [[nodiscard]] Column<std::string>& interfaceInputColumn() noexcept { return m_interfaceInput; }
    [[nodiscard]] Column<base::EntityId>& fluidSetIdColumn() noexcept { return m_fluidSetId; }
    [[nodiscard]] Column<base::EntityId>& structureSetIdColumn() noexcept { return m_structureSetId; }
    [[nodiscard]] Column<double>& normalToleranceColumn() noexcept { return m_normalTolerance; }
    [[nodiscard]] Column<std::string>& methodNameColumn() noexcept { return m_methodName; }
    [[nodiscard]] Column<double>& skinToleranceColumn() noexcept { return m_skinTolerance; }
    [[nodiscard]] Column<double>& skinToleranceIncrementColumn() noexcept { return m_skinToleranceIncrement; }
    [[nodiscard]] Column<double>& interfaceToleranceColumn() noexcept { return m_interfaceTolerance; }
    [[nodiscard]] Column<std::string>& wholeStructureSetColumn() noexcept { return m_wholeStructureSet; }
    [[nodiscard]] Column<std::string>& searchUnitColumn() noexcept { return m_searchUnit; }

private:
    Column<std::string> m_interfaceKind;
    Column<std::string> m_interfaceInput;
    Column<base::EntityId> m_fluidSetId;
    Column<base::EntityId> m_structureSetId;
    Column<double> m_normalTolerance;
    Column<std::string> m_methodName;
    Column<double> m_skinTolerance;
    Column<double> m_skinToleranceIncrement;
    Column<double> m_interfaceTolerance;
    Column<std::string> m_wholeStructureSet;
    Column<std::string> m_searchUnit;
};

} // namespace oor::model
