#pragma once

#include "oor/model/cards/properties/massProp/MassPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MassPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MassPropCard& card);
    // One property, or up to four when the later pairs of fields are there.
    [[nodiscard]] static std::vector<MassPropCard> readAll(FieldReader& reader);
    static void write(const MassPropCard& card, FieldWriter& writer);
};

// All the PMASS properties, one row each.
class MassPropTable final : public TypedTable<MassPropTable, MassPropCard>
{
public:
    static constexpr std::string_view NAME = "MassProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = MassPropFormat;

    [[nodiscard]] static base::EntityId cardId(const MassPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const MassPropCard& card, base::CardKey key);
    [[nodiscard]] MassPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MassPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_mass;
};

} // namespace oor::model
