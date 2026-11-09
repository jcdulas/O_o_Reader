#pragma once

#include "oor/model/cards/elements/fastener/FastenerCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FastenerFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FastenerCard& card);
    [[nodiscard]] static std::optional<FastenerCard> read(FieldReader& reader);
    static void write(const FastenerCard& card, FieldWriter& writer);
};

// All the CFAST elements. The location is an (N, 3) NumPy view, NaN where blank.
class FastenerTable final : public TypedTable<FastenerTable, FastenerCard>
{
public:
    static constexpr std::string_view NAME = "Fastener";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = FastenerFormat;

    FastenerTable()
        : m_kind(std::string("Property")),
          m_location(std::array<double, 3>{FastenerCard::NO_COORDINATE, FastenerCard::NO_COORDINATE,
                                           FastenerCard::NO_COORDINATE})
    {
    }

    [[nodiscard]] static base::EntityId cardId(const FastenerCard& card) noexcept { return card.id; }
    std::uint32_t append(const FastenerCard& card, base::CardKey key);
    [[nodiscard]] FastenerCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FastenerCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& patchIdAColumn() noexcept { return m_patchIdA; }
    [[nodiscard]] Column<base::EntityId>& patchIdBColumn() noexcept { return m_patchIdB; }
    [[nodiscard]] Column<base::EntityId>& spotNodeColumn() noexcept { return m_spotNode; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::array<double, 3>>& locationColumn() noexcept { return m_location; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::string> m_kind;
    Column<base::EntityId> m_patchIdA;
    Column<base::EntityId> m_patchIdB;
    Column<base::EntityId> m_spotNode;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::array<double, 3>> m_location;
};

} // namespace oor::model
