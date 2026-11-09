#pragma once

#include "oor/model/cards/elements/solidTetra/SolidTetraCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SolidTetraFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidTetraCard& card);
    [[nodiscard]] static std::optional<SolidTetraCard> read(FieldReader& reader);
    static void write(const SolidTetraCard& card, FieldWriter& writer);
};

// All the CTETRA elements: the nodes are one column of ten IDs per element, 0 for an absent edge point (an (N, 10)
// NumPy view without copy).
class SolidTetraTable final : public TypedTable<SolidTetraTable, SolidTetraCard>
{
public:
    static constexpr std::string_view NAME = "SolidTetra";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SolidTetraFormat;

    [[nodiscard]] static base::EntityId cardId(const SolidTetraCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidTetraCard& card, base::CardKey key);
    [[nodiscard]] SolidTetraCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SolidTetraCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, SolidTetraCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, SolidTetraCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
