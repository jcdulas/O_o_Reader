#pragma once

#include "oor/model/cards/superelements/freeBoundaryDofs/FreeBoundaryDofsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct FreeBoundaryDofsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeBoundaryDofsCard& card);
    [[nodiscard]] static std::optional<FreeBoundaryDofsCard> read(FieldReader& reader);
    static void write(const FreeBoundaryDofsCard& card, FieldWriter& writer);
};

// All the BNDFREE cards. A card has no ID (IdSpace::None, category Set, like the CSET it equals). The points and the
// components of every card are two lists in CSR form.
class FreeBoundaryDofsTable final : public TypedTable<FreeBoundaryDofsTable, FreeBoundaryDofsCard>
{
public:
    static constexpr std::string_view NAME = "FreeBoundaryDofs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = FreeBoundaryDofsFormat;

    [[nodiscard]] static base::EntityId cardId(const FreeBoundaryDofsCard&) noexcept { return 0; }
    std::uint32_t append(const FreeBoundaryDofsCard& card, base::CardKey key);
    [[nodiscard]] FreeBoundaryDofsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeBoundaryDofsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }

private:
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
};

} // namespace oor::model
