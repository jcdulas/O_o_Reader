#pragma once

#include "oor/model/cards/optimisation/designVar/DesignVarCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DesignVarFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignVarCard& card);
    [[nodiscard]] static std::optional<DesignVarCard> read(FieldReader& reader);
    static void write(const DesignVarCard& card, FieldWriter& writer);
};

// All the DESVAR cards. The design variables have an ID space of their own (unique per type, category DesignVariable):
// the relations, DLINK, DVGRID and the responses name a variable by it. The DDVAL field, when it is not blank, names a
// DDVAL card (DiscreteValueSet).
class DesignVarTable final : public TypedTable<DesignVarTable, DesignVarCard>
{
public:
    static constexpr std::string_view NAME = "DesignVar";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignVariable, base::IdSpace::PerCardType};
    using Format = DesignVarFormat;

    // A DELXV that is blank is NaN in its column.
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();

    DesignVarTable()
        : m_lowerBound(DesignVarCard::NO_LOWER_BOUND),
          m_upperBound(DesignVarCard::NO_UPPER_BOUND),
          m_designVarMoveLimit(BLANK)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const DesignVarCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignVarCard& card, base::CardKey key);
    [[nodiscard]] DesignVarCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DesignVarCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& entryLabelColumn() noexcept { return m_entryLabel; }
    [[nodiscard]] Column<double>& initialValueColumn() noexcept { return m_initialValue; }
    [[nodiscard]] Column<double>& lowerBoundColumn() noexcept { return m_lowerBound; }
    [[nodiscard]] Column<double>& upperBoundColumn() noexcept { return m_upperBound; }
    [[nodiscard]] Column<double>& designVarMoveLimitColumn() noexcept { return m_designVarMoveLimit; }
    [[nodiscard]] Column<base::EntityId>& discreteValuesIdColumn() noexcept { return m_discreteValuesId; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_entryLabel;
    Column<double> m_initialValue;
    Column<double> m_lowerBound;
    Column<double> m_upperBound;
    Column<double> m_designVarMoveLimit;
    Column<base::EntityId> m_discreteValuesId;
};

} // namespace oor::model
