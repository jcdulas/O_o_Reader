#pragma once

#include "oor/model/cards/nodes/moduleContactDetectParams/ModuleContactDetectParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct ModuleContactDetectParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleContactDetectParamsCard& card);
    [[nodiscard]] static std::optional<ModuleContactDetectParamsCard> read(FieldReader& reader);
    static void write(const ModuleContactDetectParamsCard& card, FieldWriter& writer);

    // The parameters the card allows: its own and those of BCPARA, BCONPRP and BCONPRG, each with what it holds.
    [[nodiscard]] static std::span<const ContactParameterRule> rules();
};

// All the MDBCATP cards. The card has no ID (IdSpace::None, category Other) and refers to no card: a table ID in FRIC
// may be a TABLEM1 or a TABL3D, so it is left alone. The format reference allows one card in a deck, which finishRead
// reports. The cards are kept whole.
class ModuleContactDetectParamsTable final
    : public TypedTable<ModuleContactDetectParamsTable, ModuleContactDetectParamsCard>
{
public:
    static constexpr std::string_view NAME = "ModuleContactDetectParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleContactDetectParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleContactDetectParamsCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleContactDetectParamsCard& card, base::CardKey key);
    [[nodiscard]] ModuleContactDetectParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleContactDetectParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no reference
    // A second MDBCATP is reported.
    void finishRead(Model& model, const ReadContext& context) override;

private:
    CardRows<ModuleContactDetectParamsCard> m_cards;
};

} // namespace oor::model
