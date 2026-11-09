#pragma once

#include "oor/model/cards/loads/metadataGroup/MetadataGroupCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MetadataGroupFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MetadataGroupCard& card);
    [[nodiscard]] static std::optional<MetadataGroupCard> read(FieldReader& reader);
    static void write(const MetadataGroupCard& card, FieldWriter& writer);
};

// All the ABSTRCT cards. The ID is the ID of the group (ABSTID), shared by the cards that add entities to it
// (IdSpace::Group). The format reference lists the card with the loads, but it is meta data: its category is
// MetadataGroup (its own kind of ID, not a load set), which the ABSNMVB, ABSNMVI, ABSNMVR and ABSNMVS cards name by
// their ABSTID
// (`ReferenceTarget::MetadataGroup`, `Model.renumber("metadata_group")`).
class MetadataGroupTable final : public TypedTable<MetadataGroupTable, MetadataGroupCard>
{
public:
    static constexpr std::string_view NAME = "MetadataGroup";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MetadataGroup, base::IdSpace::Group};
    using Format = MetadataGroupFormat;

    [[nodiscard]] static base::EntityId cardId(const MetadataGroupCard& card) noexcept { return card.id; }
    std::uint32_t append(const MetadataGroupCard& card, base::CardKey key);
    [[nodiscard]] MetadataGroupCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MetadataGroupCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::string>& groupNameColumn() noexcept { return m_groupName; }
    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& entityNameColumn() noexcept { return m_entityName; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& entitiesColumn() noexcept { return m_entities; }

private:
    Column<std::string> m_groupName;
    Column<base::EntityId> m_id;
    Column<std::string> m_entityName;
    VariableLengthColumn<base::EntityId> m_entities;
};

} // namespace oor::model
