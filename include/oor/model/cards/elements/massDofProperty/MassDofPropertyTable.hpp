#pragma once

#include "oor/model/cards/elements/massDofProperty/MassDofPropertyCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MassDofPropertyFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MassDofPropertyCard& card);
    [[nodiscard]] static std::optional<MassDofPropertyCard> read(FieldReader& reader);
    static void write(const MassDofPropertyCard& card, FieldWriter& writer);
};

// All the CMASS1 elements.
class MassDofPropertyTable final : public TypedTable<MassDofPropertyTable, MassDofPropertyCard>
{
public:
    static constexpr std::string_view NAME = "MassDofProperty";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = MassDofPropertyFormat;

    MassDofPropertyTable() : m_node1(0), m_components1(0), m_node2(0), m_components2(0) {}

    [[nodiscard]] static base::EntityId cardId(const MassDofPropertyCard& card) noexcept { return card.id; }
    std::uint32_t append(const MassDofPropertyCard& card, base::CardKey key);
    [[nodiscard]] MassDofPropertyCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MassDofPropertyCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<std::int32_t>& components1Column() noexcept { return m_components1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }
    [[nodiscard]] Column<std::int32_t>& components2Column() noexcept { return m_components2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_node1;
    Column<std::int32_t> m_components1;
    Column<base::EntityId> m_node2;
    Column<std::int32_t> m_components2;
};

} // namespace oor::model
