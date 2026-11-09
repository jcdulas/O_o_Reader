#pragma once

#include "oor/model/cards/elements/conicalShellProp/ConicalShellPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ConicalShellPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ConicalShellPropCard& card);
    [[nodiscard]] static std::optional<ConicalShellPropCard> read(FieldReader& reader);
    static void write(const ConicalShellPropCard& card, FieldWriter& writer);
};

// All the PCONEAX properties.
class ConicalShellPropTable final : public TypedTable<ConicalShellPropTable, ConicalShellPropCard>
{
public:
    static constexpr std::string_view NAME = "ConicalShellProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ConicalShellPropFormat;

    ConicalShellPropTable()
        : m_fiberDistance1(ConicalShellPropCard::NO_VALUE), m_fiberDistance2(ConicalShellPropCard::NO_VALUE)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const ConicalShellPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const ConicalShellPropCard& card, base::CardKey key);
    [[nodiscard]] ConicalShellPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ConicalShellPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matId1Column() noexcept { return m_matId1; }
    [[nodiscard]] Column<double>& thickness1Column() noexcept { return m_thickness1; }
    [[nodiscard]] Column<base::EntityId>& matId2Column() noexcept { return m_matId2; }
    [[nodiscard]] Column<double>& inertiaColumn() noexcept { return m_inertia; }
    [[nodiscard]] Column<base::EntityId>& matId3Column() noexcept { return m_matId3; }
    [[nodiscard]] Column<double>& thickness2Column() noexcept { return m_thickness2; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& fiberDistance1Column() noexcept { return m_fiberDistance1; }
    [[nodiscard]] Column<double>& fiberDistance2Column() noexcept { return m_fiberDistance2; }
    [[nodiscard]] VariableLengthColumn<double>& phiAngleColumn() noexcept { return m_phiAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId1;
    Column<double> m_thickness1;
    Column<base::EntityId> m_matId2;
    Column<double> m_inertia;
    Column<base::EntityId> m_matId3;
    Column<double> m_thickness2;
    Column<double> m_nonStructMass;
    Column<double> m_fiberDistance1;
    Column<double> m_fiberDistance2;
    VariableLengthColumn<double> m_phiAngle;
};

} // namespace oor::model
