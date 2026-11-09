#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/elements/seam/SeamCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct SeamFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SeamCard& card);
    [[nodiscard]] static std::optional<SeamCard> read(FieldReader& reader);
    static void write(const SeamCard& card, FieldWriter& writer);
};

// All the CSEAM elements: an element in the space every element shares (IdSpace::Element). The grid points GS and GE
// are one column of two IDs, `nodes` (0 in the coordinate form), and the coordinates are two columns of three reals,
// `start` and `end` (NaN for a blank one); the form is the column `ends`. PID names a PSEAM; the four patch numbers
// name properties or elements as CTYPE says.
class SeamTable final : public TypedTable<SeamTable, SeamCard>
{
public:
    static constexpr std::string_view NAME = "Seam";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SeamFormat;
    static constexpr std::array<double, 3> BLANK_XYZ = {BLANK_REAL, BLANK_REAL, BLANK_REAL};

    SeamTable()
        : m_patchKind(static_cast<std::uint8_t>(SeamPatchType::Property)),
          m_ends(static_cast<std::uint8_t>(SeamEnds::GridPoints)),
          m_start(BLANK_XYZ),
          m_end(BLANK_XYZ)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const SeamCard& card) noexcept { return card.id; }
    std::uint32_t append(const SeamCard& card, base::CardKey key);
    [[nodiscard]] SeamCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SeamCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::string>& seamLineColumn() noexcept { return m_seamLine; }
    [[nodiscard]] Column<std::uint8_t>& patchKindColumn() noexcept { return m_patchKind; }
    [[nodiscard]] Column<base::EntityId>& patchAStartColumn() noexcept { return m_patchAStart; }
    [[nodiscard]] Column<base::EntityId>& patchBStartColumn() noexcept { return m_patchBStart; }
    [[nodiscard]] Column<base::EntityId>& patchAEndColumn() noexcept { return m_patchAEnd; }
    [[nodiscard]] Column<base::EntityId>& patchBEndColumn() noexcept { return m_patchBEnd; }
    [[nodiscard]] Column<std::uint8_t>& endsColumn() noexcept { return m_ends; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<std::array<double, 3>>& startColumn() noexcept { return m_start; }
    [[nodiscard]] Column<std::array<double, 3>>& endColumn() noexcept { return m_end; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::string> m_seamLine;
    Column<std::uint8_t> m_patchKind;
    Column<base::EntityId> m_patchAStart;
    Column<base::EntityId> m_patchBStart;
    Column<base::EntityId> m_patchAEnd;
    Column<base::EntityId> m_patchBEnd;
    Column<std::uint8_t> m_ends;
    Column<std::array<base::EntityId, 2>> m_nodes;
    Column<std::array<double, 3>> m_start;
    Column<std::array<double, 3>> m_end;
};

} // namespace oor::model
