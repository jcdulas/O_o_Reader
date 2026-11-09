#pragma once

#include "oor/model/cards/nonlinear_contact/boltConnection/BoltConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BoltConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BoltConnectionCard& card);
    [[nodiscard]] static std::optional<BoltConnectionCard> read(FieldReader& reader);
    static void write(const BoltConnectionCard& card, FieldWriter& writer);
};

// All the BOLT cards. The format reference calls the ID an element ID, but a bolt makes no element of the model: the ID
// is unique among the BOLT cards (IdSpace::PerCardType, category Other). The grid points are references.
class BoltConnectionTable final : public TypedTable<BoltConnectionTable, BoltConnectionCard>
{
public:
    static constexpr std::string_view NAME = "BoltConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = BoltConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const BoltConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const BoltConnectionCard& card, base::CardKey key);
    [[nodiscard]] BoltConnectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BoltConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& topNodesColumn() noexcept { return m_topNodes; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& bottomNodesColumn() noexcept { return m_bottomNodes; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_controlNode;
    VariableLengthColumn<base::EntityId> m_topNodes;
    VariableLengthColumn<base::EntityId> m_bottomNodes;
};

} // namespace oor::model
