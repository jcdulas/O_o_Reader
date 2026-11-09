#pragma once

#include "oor/model/cards/explicit/initialConditionValues/InitialConditionValuesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialConditionValuesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialConditionValuesCard& card);
    [[nodiscard]] static std::optional<InitialConditionValuesCard> read(FieldReader& reader);
    static void write(const InitialConditionValuesCard& card, FieldWriter& writer);
};

// All the TICVAL cards. The TSID is the ID of the table (IdSpace::PerCardType, category InitialValueSet): a TICREG
// names it. In a radial list the values of the variables that have a profile are TABLED1 IDs and are visited as such.
class InitialConditionValuesTable final : public TypedTable<InitialConditionValuesTable, InitialConditionValuesCard>
{
public:
    static constexpr std::string_view NAME = "InitialConditionValues";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialValueSet, base::IdSpace::PerCardType};
    using Format = InitialConditionValuesFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialConditionValuesCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialConditionValuesCard& card, base::CardKey key);
    [[nodiscard]] InitialConditionValuesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialConditionValuesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialConditionValuesCard> m_cards;
};

} // namespace oor::model
