#pragma once

#include "oor/model/cards/elements/solidHexa/SolidHexaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SolidHexaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SolidHexaCard& card);
    [[nodiscard]] static std::optional<SolidHexaCard> read(FieldReader& reader);
    static void write(const SolidHexaCard& card, FieldWriter& writer);
};

// All the CHEXA elements: the nodes are one column of twenty IDs per element, 0 for an absent edge point (an (N, 20)
// NumPy view without copy).
class SolidHexaTable final : public TypedTable<SolidHexaTable, SolidHexaCard>
{
public:
    static constexpr std::string_view NAME = "SolidHexa";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SolidHexaFormat;

    [[nodiscard]] static base::EntityId cardId(const SolidHexaCard& card) noexcept { return card.id; }
    std::uint32_t append(const SolidHexaCard& card, base::CardKey key);
    [[nodiscard]] SolidHexaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SolidHexaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, SolidHexaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, SolidHexaCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
