#pragma once

#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AeroRigidTrimVariableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroRigidTrimVariableCard& card);
    [[nodiscard]] static std::optional<AeroRigidTrimVariableCard> read(FieldReader& reader);
    static void write(const AeroRigidTrimVariableCard& card, FieldWriter& writer);
};

// All the AESTAT cards, in the category AeroTrimVariable that AEPARM shares. The ID is ignored by the solver, so the
// space is IdSpace::Group: several cards may carry the same number, and `model.aestat[5]` is the list of the cards of
// that ID. The card names no other card by number; a TRIM, AELINK or UXVEC finds it by its label.
class AeroRigidTrimVariableTable final : public TypedTable<AeroRigidTrimVariableTable, AeroRigidTrimVariableCard>
{
public:
    static constexpr std::string_view NAME = "AeroRigidTrimVariable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroTrimVariable, base::IdSpace::Group};
    using Format = AeroRigidTrimVariableFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroRigidTrimVariableCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroRigidTrimVariableCard& card, base::CardKey key);
    [[nodiscard]] AeroRigidTrimVariableCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroRigidTrimVariableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_label;
};

} // namespace oor::model
