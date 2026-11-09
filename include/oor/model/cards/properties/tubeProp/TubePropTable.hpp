#pragma once

#include "oor/model/cards/properties/tubeProp/TubePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TubePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TubePropCard& card);
    [[nodiscard]] static std::optional<TubePropCard> read(FieldReader& reader);
    static void write(const TubePropCard& card, FieldWriter& writer);
};

class TubePropTable final : public TypedTable<TubePropTable, TubePropCard>
{
public:
    static constexpr std::string_view NAME = "TubeProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = TubePropFormat;

    [[nodiscard]] static base::EntityId cardId(const TubePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const TubePropCard& card, base::CardKey key);
    [[nodiscard]] TubePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TubePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& outerDiameterColumn() noexcept { return m_outerDiameter; }
    [[nodiscard]] Column<double>& thicknessColumn() noexcept { return m_thickness; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<double>& outerDiameter2Column() noexcept { return m_outerDiameter2; }
    [[nodiscard]] Column<std::uint8_t>& thicknessGivenColumn() noexcept { return m_thicknessGiven; }
    [[nodiscard]] Column<std::uint8_t>& outerDiameter2GivenColumn() noexcept { return m_outerDiameter2Given; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_outerDiameter;
    Column<double> m_thickness;
    Column<double> m_nonStructMass;
    Column<double> m_outerDiameter2;
    Column<std::uint8_t> m_thicknessGiven;
    Column<std::uint8_t> m_outerDiameter2Given;
};

} // namespace oor::model
