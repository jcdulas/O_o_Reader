#pragma once

#include "oor/model/cards/sets/aeroPrismNodeSet2/AeroPrismNodeSet2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroPrismNodeSet2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroPrismNodeSet2Card& card);
    [[nodiscard]] static std::optional<AeroPrismNodeSet2Card> read(FieldReader& reader);
    static void write(const AeroPrismNodeSet2Card& card, FieldWriter& writer);
};

// All the SET2 cards, in the category Set. The ID is a set ID shared by the cards that have it (IdSpace::Group), so the
// ReferenceTarget::Set of a SPLINEi finds it as it finds a SET1 or a SET3. The MACRO is a reference to a CAERO.
class AeroPrismNodeSet2Table final : public TypedTable<AeroPrismNodeSet2Table, AeroPrismNodeSet2Card>
{
public:
    static constexpr std::string_view NAME = "AeroPrismNodeSet2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::Group};
    using Format = AeroPrismNodeSet2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroPrismNodeSet2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroPrismNodeSet2Card& card, base::CardKey key);
    [[nodiscard]] AeroPrismNodeSet2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroPrismNodeSet2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& aeroPanelIdColumn() noexcept { return m_aeroPanelId; }
    [[nodiscard]] Column<double>& spanStartColumn() noexcept { return m_spanStart; }
    [[nodiscard]] Column<double>& spanEndColumn() noexcept { return m_spanEnd; }
    [[nodiscard]] Column<double>& chordStartColumn() noexcept { return m_chordStart; }
    [[nodiscard]] Column<double>& chordEndColumn() noexcept { return m_chordEnd; }
    [[nodiscard]] Column<double>& zMaximumColumn() noexcept { return m_zMaximum; }
    [[nodiscard]] Column<double>& zMinimumColumn() noexcept { return m_zMinimum; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_aeroPanelId;
    Column<double> m_spanStart;
    Column<double> m_spanEnd;
    Column<double> m_chordStart;
    Column<double> m_chordEnd;
    Column<double> m_zMaximum;
    Column<double> m_zMinimum;
};

} // namespace oor::model
