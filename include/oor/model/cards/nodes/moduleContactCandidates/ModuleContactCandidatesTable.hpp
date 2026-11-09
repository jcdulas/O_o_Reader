#pragma once

#include "oor/model/cards/nodes/moduleContactCandidates/ModuleContactCandidatesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleContactCandidatesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleContactCandidatesCard& card);
    [[nodiscard]] static std::optional<ModuleContactCandidatesCard> read(FieldReader& reader);
    static void write(const ModuleContactCandidatesCard& card, FieldWriter& writer);
};

// All the MDBCPAR cards. The card has no ID (IdSpace::None, category Other); its SET3ID, when it is above zero, is a
// reference to a SET3 (the modules of its list). The format reference allows one card in a deck, which finishRead
// reports. The cards are kept whole.
class ModuleContactCandidatesTable final : public TypedTable<ModuleContactCandidatesTable, ModuleContactCandidatesCard>
{
public:
    static constexpr std::string_view NAME = "ModuleContactCandidates";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleContactCandidatesFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleContactCandidatesCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleContactCandidatesCard& card, base::CardKey key);
    [[nodiscard]] ModuleContactCandidatesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleContactCandidatesCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // A second MDBCPAR is reported.
    void finishRead(Model& model, const ReadContext& context) override;

private:
    CardRows<ModuleContactCandidatesCard> m_cards;
};

} // namespace oor::model
