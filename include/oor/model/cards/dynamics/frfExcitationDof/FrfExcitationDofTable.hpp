#pragma once

#include "oor/model/cards/dynamics/frfExcitationDof/FrfExcitationDofCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FrfExcitationDofFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfExcitationDofCard& card);
    [[nodiscard]] static std::optional<FrfExcitationDofCard> read(FieldReader& reader);
    static void write(const FrfExcitationDofCard& card, FieldWriter& writer);
};

// All the FRFXIT cards. A card has no ID of its own (IdSpace::None, category Other), so there may be many. The PNTID is
// a grid point or a scalar point of the model (ReferenceTarget::ScalarOrGridPoint: a scalar point that no SPOINT
// defines is not reported).
class FrfExcitationDofTable final : public TypedTable<FrfExcitationDofTable, FrfExcitationDofCard>
{
public:
    static constexpr std::string_view NAME = "FrfExcitationDof";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FrfExcitationDofFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfExcitationDofCard&) noexcept { return 0; }
    std::uint32_t append(const FrfExcitationDofCard& card, base::CardKey key);
    [[nodiscard]] FrfExcitationDofCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FrfExcitationDofCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& pointIdColumn() noexcept { return m_pointId; }
    [[nodiscard]] Column<std::int32_t>& componentColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::string>& entryLabelColumn() noexcept { return m_entryLabel; }

private:
    Column<base::EntityId> m_pointId;
    Column<std::int32_t> m_components;
    Column<std::string> m_entryLabel;
};

} // namespace oor::model
