#pragma once

#include "oor/model/cards/nonlinear_contact/crackClosureCrack/CrackClosureCrackCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CrackClosureCrackFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrackClosureCrackCard& card);
    [[nodiscard]] static std::optional<CrackClosureCrackCard> read(FieldReader& reader);
    static void write(const CrackClosureCrackCard& card, FieldWriter& writer);
};

// All the VCCT cards (category Other). The ID is the crack set that a Case Control VCCT names, so the cards of a set
// share it (IdSpace::Group); IDCR numbers the crack inside the set. The four tables are TABLEMi
// (ReferenceTarget::MaterialTable), the grid points of the crack front are ReferenceTarget::Node (the two ends of a
// THRU range are NodeInRange) and a negative G1 is the ID of a SET3 (ReferenceTarget::Set). The cards are kept whole
// next to a column of their IDs.
class CrackClosureCrackTable final : public TypedTable<CrackClosureCrackTable, CrackClosureCrackCard>
{
public:
    static constexpr std::string_view NAME = "CrackClosureCrack";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = CrackClosureCrackFormat;

    [[nodiscard]] static base::EntityId cardId(const CrackClosureCrackCard& card) noexcept { return card.setId; }
    std::uint32_t append(const CrackClosureCrackCard& card, base::CardKey key);
    [[nodiscard]] CrackClosureCrackCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CrackClosureCrackCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<CrackClosureCrackCard> m_cards;
};

} // namespace oor::model
