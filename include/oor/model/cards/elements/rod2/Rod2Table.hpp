#pragma once

#include "oor/model/cards/elements/rod2/Rod2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct Rod2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Rod2Card& card);
    [[nodiscard]] static std::optional<Rod2Card> read(FieldReader& reader);
    static void write(const Rod2Card& card, FieldWriter& writer);
};

class Rod2Table final : public TypedTable<Rod2Table, Rod2Card>
{
public:
    static constexpr std::string_view NAME = "Rod2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Rod2Format;

    [[nodiscard]] static base::EntityId cardId(const Rod2Card& card) noexcept { return card.id; }
    std::uint32_t append(const Rod2Card& card, base::CardKey key);
    [[nodiscard]] Rod2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const Rod2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] base::EntityId propId(std::uint32_t row) const { return m_propId.get(row); }
    [[nodiscard]] std::array<base::EntityId, 2> nodes(std::uint32_t row) const { return m_nodes.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 2>> m_nodes;
};

} // namespace oor::model
