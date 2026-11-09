#pragma once

#include "oor/model/cards/elements/shellConical2/ShellConical2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ShellConical2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellConical2Card& card);
    [[nodiscard]] static std::optional<ShellConical2Card> read(FieldReader& reader);
    static void write(const ShellConical2Card& card, FieldWriter& writer);
};

// All the CCONEAX elements.
class ShellConical2Table final : public TypedTable<ShellConical2Table, ShellConical2Card>
{
public:
    static constexpr std::string_view NAME = "ShellConical2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShellConical2Format;

    [[nodiscard]] static base::EntityId cardId(const ShellConical2Card& card) noexcept { return card.id; }
    std::uint32_t append(const ShellConical2Card& card, base::CardKey key);
    [[nodiscard]] ShellConical2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellConical2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& ringIdAColumn() noexcept { return m_ringIdA; }
    [[nodiscard]] Column<base::EntityId>& ringIdBColumn() noexcept { return m_ringIdB; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_ringIdA;
    Column<base::EntityId> m_ringIdB;
};

} // namespace oor::model
