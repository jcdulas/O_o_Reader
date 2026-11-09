#pragma once

#include "oor/model/cards/aero/aeroComponent/AeroComponentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AeroComponentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroComponentCard& card);
    [[nodiscard]] static std::optional<AeroComponentCard> read(FieldReader& reader);
    static void write(const AeroComponentCard& card, FieldWriter& writer);
};

// The kind of card the IDs of an AECOMP of this LISTTYPE name: a SET1 or SET3 (Set), an AELIST (AeroList), an
// aerodynamic element (AeroElement). Nothing for CMPID: the component number of an AEQUAD4 or AETRIA3 is no card's ID.
[[nodiscard]] std::optional<ReferenceTarget> aeroComponentTarget(AeroComponentListType type) noexcept;

// All the AECOMP cards, in the category AeroComponent that AECOMPL shares. A component has no number
// (IdSpace::None): it is found by its NAME (`model.aecomp["WING"]`, matched without regard to the case), which is
// unique among the AECOMP and AECOMPL cards (a check between cards). The cards are kept whole next to a column of their
// names.
class AeroComponentTable final : public TypedTable<AeroComponentTable, AeroComponentCard>
{
public:
    static constexpr std::string_view NAME = "AeroComponent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroComponent, base::IdSpace::None};
    using Format = AeroComponentFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroComponentCard&) noexcept { return 0; }
    std::uint32_t append(const AeroComponentCard& card, base::CardKey key);
    [[nodiscard]] AeroComponentCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroComponentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // The IDs are the cards the LISTTYPE says (aeroComponentTarget), field "listIdValue".
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The name of a row, as written, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_name.get(row); }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<AeroComponentCard> m_cards;
};

} // namespace oor::model
