#pragma once

#include "oor/model/cards/properties/planeProp/PlanePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PlanePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlanePropCard& card);
    [[nodiscard]] static std::optional<PlanePropCard> read(FieldReader& reader);
    static void write(const PlanePropCard& card, FieldWriter& writer);
};

// All the PPLANE cards: properties (IdSpace::Property), so the PID of a plane element finds them. The column of the
// thickness holds NaN for a blank T (aero/BlankReal.hpp).
class PlanePropTable final : public TypedTable<PlanePropTable, PlanePropCard>
{
public:
    static constexpr std::string_view NAME = "PlaneProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = PlanePropFormat;

    [[nodiscard]] static base::EntityId cardId(const PlanePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlanePropCard& card, base::CardKey key);
    [[nodiscard]] PlanePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlanePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& tColumn() noexcept { return m_t; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }
    [[nodiscard]] Column<std::int32_t>& formulationOptionColumn() noexcept { return m_formulationOption; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_t;
    Column<double> m_nonStructMass;
    Column<std::int32_t> m_formulationOption;
};

} // namespace oor::model
