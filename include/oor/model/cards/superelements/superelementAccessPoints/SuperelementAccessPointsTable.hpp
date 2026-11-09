#pragma once

#include "oor/model/cards/superelements/superelementAccessPoints/SuperelementAccessPointsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementAccessPointsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementAccessPointsCard& card);
    [[nodiscard]] static std::optional<SuperelementAccessPointsCard> read(FieldReader& reader);
    static void write(const SuperelementAccessPointsCard& card, FieldWriter& writer);
};

// All the ACCSSPT cards (no ID: IdSpace::None, category Other, so a deck may repeat the card as often as it likes). The
// points of the cards are one column in CSR form; each is a grid point (ReferenceTarget::Node), so a renumbering of the
// nodes follows into the list.
class SuperelementAccessPointsTable final
    : public TypedTable<SuperelementAccessPointsTable, SuperelementAccessPointsCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementAccessPoints";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = SuperelementAccessPointsFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementAccessPointsCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementAccessPointsCard& card, base::CardKey key);
    [[nodiscard]] SuperelementAccessPointsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementAccessPointsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeIdsColumn() noexcept { return m_nodeIds; }

private:
    VariableLengthColumn<base::EntityId> m_nodeIds;
};

} // namespace oor::model
