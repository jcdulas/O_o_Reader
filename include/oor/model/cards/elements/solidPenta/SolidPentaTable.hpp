#pragma once

#include "oor/model/cards/elements/solidPenta/SolidPentaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SolidPentaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidPentaCard& card);
    [[nodiscard]] static std::optional<SolidPentaCard> read(FieldReader& reader);
    static void write(const SolidPentaCard& card, FieldWriter& writer);
};

// All the CPENTA elements: the nodes are one column of fifteen IDs per element, 0 for an absent edge point (an (N, 15)
// NumPy view without copy).
class SolidPentaTable final : public TypedTable<SolidPentaTable, SolidPentaCard>
{
public:
    static constexpr std::string_view NAME = "SolidPenta";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SolidPentaFormat;

    [[nodiscard]] static base::EntityId cardId(const SolidPentaCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidPentaCard& card, base::CardKey key);
    [[nodiscard]] SolidPentaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SolidPentaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, SolidPentaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, SolidPentaCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
