#pragma once

#include "oor/model/cards/explicit/explicitParam/ExplicitParamCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ExplicitParamFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing): the rules of the name
    // and of every word and number, then, for a name the catalogue lists, the layout of its entry.
    [[nodiscard]] static std::optional<CardProblem> check(const ExplicitParamCard& card);
    [[nodiscard]] static std::optional<ExplicitParamCard> read(FieldReader& reader);
    static void write(const ExplicitParamCard& card, FieldWriter& writer);
};

// All the DYPARAM cards. A DYPARAM has no number (IdSpace::None, category Other): it is found by its name, and the
// model keeps every card (a deck gives CONTACT, SMP and MATRMERG several times, each a separate setting). Two cards of
// one setting are not reported at read time (the model only keeps the errors a table's finishRead finds, and the format
// reference does not call this an error): `explicitParamIdentity` tells the setting a card gives, for a validation
// check. The cards are kept whole (CardRows) next to their name, which is what the query engine reads.
class ExplicitParamTable final : public TypedTable<ExplicitParamTable, ExplicitParamCard>
{
public:
    static constexpr std::string_view NAME = "ExplicitParam";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ExplicitParamFormat;

    [[nodiscard]] static base::EntityId cardId(const ExplicitParamCard&) noexcept { return 0; }
    std::uint32_t append(const ExplicitParamCard& card, base::CardKey key);
    [[nodiscard]] ExplicitParamCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ExplicitParamCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // The tables of COSUBMXT, the grid points of CONTACT,INFO and the MATRIGs of MATRMERG and MATRMRG1.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] std::string nameAtRow(std::uint32_t row) const { return m_entryName.get(row); }
    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }

private:
    Column<std::string> m_entryName;
    CardRows<ExplicitParamCard> m_cards;
};

} // namespace oor::model
