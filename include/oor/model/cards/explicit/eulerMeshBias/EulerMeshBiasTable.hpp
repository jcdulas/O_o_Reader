#pragma once

#include "oor/model/cards/explicit/eulerMeshBias/EulerMeshBiasCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerMeshBiasFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerMeshBiasCard& card);
    [[nodiscard]] static std::optional<EulerMeshBiasCard> read(FieldReader& reader);
    static void write(const EulerMeshBiasCard& card, FieldWriter& writer);
};

// All the BIAS cards. The BID is unique among the BIAS cards (IdSpace::PerCardType) and is in the category MeshBias,
// which the IBIDX, IBIDY and IBIDZ of a MESH name. A BIAS names no other card.
class EulerMeshBiasTable final : public TypedTable<EulerMeshBiasTable, EulerMeshBiasCard>
{
public:
    static constexpr std::string_view NAME = "EulerMeshBias";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MeshBias, base::IdSpace::PerCardType};
    using Format = EulerMeshBiasFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerMeshBiasCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerMeshBiasCard& card, base::CardKey key);
    [[nodiscard]] EulerMeshBiasCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerMeshBiasCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a BIAS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerMeshBiasCard> m_cards;
};

} // namespace oor::model
