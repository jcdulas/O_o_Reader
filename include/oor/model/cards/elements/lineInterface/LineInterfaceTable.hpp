#pragma once

#include "oor/model/cards/elements/lineInterface/LineInterfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The items of the text of a LIST: "LIST=(101,102(Q),-103(Q),104(L))" gives (101, L), (102, Q), (-103, Q), (104, L).
// Blanks anywhere are ignored, the word LIST and the letters are read in any case, and an item without parentheses is
// linear. Gives the items, or nothing and the reason in `problem` when the text is not a LIST of the format (it must
// have the form LIST=(...), at least two items, each a non-zero integer and, if it has parentheses, L or Q).
[[nodiscard]] std::optional<std::vector<LineInterfaceBoundary>> parseLineInterfaceList(std::string_view text,
                                                                                           std::string& problem);

// The text of a LIST: LIST=(BID1,BID2(Q),...). The interpolation L is written in parentheses only when
// `explicitLinear` is true (otherwise it is the blank default).
[[nodiscard]] std::string formatLineInterfaceList(const std::vector<LineInterfaceBoundary>& boundaries,
                                                  bool explicitLinear);

struct LineInterfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const LineInterfaceCard& card);
    [[nodiscard]] static std::optional<LineInterfaceCard> read(FieldReader& reader);
    static void write(const LineInterfaceCard& card, FieldWriter& writer);
};

// All the CINTC cards, each kept whole (the LIST has a variable number of items). The ID is unique per card type
// (IdSpace::PerCardType, category Other): a line interface element is not an element of the structure. The BID of
// every boundary of the LIST is a GeometricBoundary reference (a GMBNDC): the sign says which boundary is the
// independent one and stays on the card, the number is what the target sees.
class LineInterfaceTable final : public TypedTable<LineInterfaceTable, LineInterfaceCard>
{
public:
    static constexpr std::string_view NAME = "LineInterface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = LineInterfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const LineInterfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const LineInterfaceCard& card, base::CardKey key);
    [[nodiscard]] LineInterfaceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const LineInterfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<LineInterfaceCard> m_cards;
};

} // namespace oor::model
