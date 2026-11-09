#pragma once

#include "oor/model/cards/elements/bushing2D/Bushing2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct Bushing2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Bushing2DCard& card);
    [[nodiscard]] static std::optional<Bushing2DCard> read(FieldReader& reader);
    static void write(const Bushing2DCard& card, FieldWriter& writer);
};

// All the CBUSH2D elements: an element ID, in the space every element shares (IdSpace::Element). PID names a PBUSH2D,
// the two points are grid points and CID is a coordinate system. The nodes are one column of two IDs per element and
// the plane is a column of text.
class Bushing2DTable final : public TypedTable<Bushing2DTable, Bushing2DCard>
{
public:
    static constexpr std::string_view NAME = "Bushing2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Bushing2DFormat;

    Bushing2DTable() : m_plane(std::string(BUSH_ELEM_2D_DEFAULT_PLANE)) {}

    [[nodiscard]] static base::EntityId cardId(const Bushing2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const Bushing2DCard& card, base::CardKey key);
    [[nodiscard]] Bushing2DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const Bushing2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, Bushing2DCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::string>& planeColumn() noexcept { return m_plane; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, Bushing2DCard::NODE_COUNT>> m_nodes;
    Column<base::EntityId> m_frameId;
    Column<std::string> m_plane;
};

} // namespace oor::model
