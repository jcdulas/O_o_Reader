#pragma once

#include "oor/model/cards/solution_control/rotorSuperelement/RotorSuperelementCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RotorSuperelementFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorSuperelementCard& card);
    [[nodiscard]] static std::optional<RotorSuperelementCard> read(FieldReader& reader);
    static void write(const RotorSuperelementCard& card, FieldWriter& writer);
};

// All the ROTORSE cards. The ID is the rotor ID (category Rotor, IdSpace::Group like ROTORG: a rotor may be made of
// several cards). The SEID is a superelement number: it is looked up as such (ReferenceTarget::Superelement).
class RotorSuperelementTable final : public TypedTable<RotorSuperelementTable, RotorSuperelementCard>
{
public:
    static constexpr std::string_view NAME = "RotorSuperelement";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Rotor, base::IdSpace::Group};
    using Format = RotorSuperelementFormat;

    RotorSuperelementTable() : m_superelementOption(RotorSuperelementCard::DEFAULT_SUPERELEMENT_OPTION) {}

    [[nodiscard]] static base::EntityId cardId(const RotorSuperelementCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorSuperelementCard& card, base::CardKey key);
    [[nodiscard]] RotorSuperelementCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RotorSuperelementCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::int32_t>& superelementOptionColumn() noexcept { return m_superelementOption; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_superelementId;
    Column<std::int32_t> m_superelementOption;
};

} // namespace oor::model
