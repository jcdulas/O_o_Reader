#pragma once

#include "oor/model/cards/optimisation/designEquation/DesignEquationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DesignEquationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing): an ID that is not
    // positive, or a text that is not a set of equations.
    [[nodiscard]] static std::optional<CardProblem> check(const DesignEquationCard& card);
    [[nodiscard]] static std::optional<DesignEquationCard> read(FieldReader& reader);
    static void write(const DesignEquationCard& card, FieldWriter& writer);
};

// All the DEQATN cards. The ID is unique among them (IdSpace::PerCardType); a DVPREL2, DVMREL2, DVCREL2 or DRESP2, a
// PBUSH1D of type EQUAT and a CORD3G of form EQN name an equation by it.
class DesignEquationTable final : public TypedTable<DesignEquationTable, DesignEquationCard>
{
public:
    static constexpr std::string_view NAME = "DesignEquation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignEquation, base::IdSpace::PerCardType};
    using Format = DesignEquationFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignEquationCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignEquationCard& card, base::CardKey key);
    [[nodiscard]] DesignEquationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignEquationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignEquationCard> m_cards;
};

} // namespace oor::model
