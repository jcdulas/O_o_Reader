#pragma once

#include "oor/model/cards/constraints/geomSinglePointConstr/GeomSinglePointConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct GeomSinglePointConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GeomSinglePointConstrCard& card);
    [[nodiscard]] static std::optional<GeomSinglePointConstrCard> read(FieldReader& reader);
    static void write(const GeomSinglePointConstrCard& card, FieldWriter& writer);
};

// All the GMSPC cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The entity ID names a
// geometric entity (FEFACE, GMCURV, ...), which is not a card of the model: it is a ReferenceTarget::Other.
class GeomSinglePointConstrTable final : public TypedTable<GeomSinglePointConstrTable, GeomSinglePointConstrCard>
{
public:
    static constexpr std::string_view NAME = "GeomSinglePointConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = GeomSinglePointConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const GeomSinglePointConstrCard& card) noexcept { return card.id; }
    std::uint32_t append(const GeomSinglePointConstrCard& card, base::CardKey key);
    [[nodiscard]] GeomSinglePointConstrCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GeomSinglePointConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::string>& entityTypeColumn() noexcept { return m_entityType; }
    [[nodiscard]] Column<base::EntityId>& entityIdColumn() noexcept { return m_entityId; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_components;
    Column<std::string> m_entityType;
    Column<base::EntityId> m_entityId;
};

} // namespace oor::model
