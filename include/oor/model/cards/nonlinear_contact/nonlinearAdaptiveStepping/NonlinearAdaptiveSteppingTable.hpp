#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearAdaptiveStepping/NonlinearAdaptiveSteppingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct NonlinearAdaptiveSteppingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearAdaptiveSteppingCard& card);
    [[nodiscard]] static std::optional<NonlinearAdaptiveSteppingCard> read(FieldReader& reader);
    static void write(const NonlinearAdaptiveSteppingCard& card, FieldWriter& writer);

    // The names of the fields, with what each holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the NLADAPT cards. The ID is the ID of the NLPARM it goes with: unique among the NLADAPT cards
// (IdSpace::PerCardType) and numbering, in the category NonlinearStatic, the same selection as that NLPARM and its
// NLPCI, so that a renumbering of the category moves them together. An NLADAPT may also go with an NLSTEP (the format
// reference allows both); that case is not followed by a renumbering of the NLSTEP IDs, and no reference check is made,
// like for an NLPCI.
class NonlinearAdaptiveSteppingTable final
    : public TypedTable<NonlinearAdaptiveSteppingTable, NonlinearAdaptiveSteppingCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearAdaptiveStepping";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearStatic, base::IdSpace::PerCardType};
    using Format = NonlinearAdaptiveSteppingFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearAdaptiveSteppingCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearAdaptiveSteppingCard& card, base::CardKey key);
    [[nodiscard]] NonlinearAdaptiveSteppingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearAdaptiveSteppingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // see the class comment: no reference check

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<NonlinearAdaptiveSteppingCard> m_cards;
};

} // namespace oor::model
