#pragma once

#include "oor/model/cards/aero/aeroDownwash/AeroDownwashCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroDownwashFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroDownwashCard& card);
    [[nodiscard]] static std::optional<AeroDownwashCard> read(FieldReader& reader);
    static void write(const AeroDownwashCard& card, FieldWriter& writer);
};

// All the AEDW cards. An AEDW has no numeric ID of its own (IdSpace::None, category Aero): there is one for each Mach
// number, symmetry and control vector, so a deck has many. The UXID names a UXVEC (ReferenceTarget::ControlVector); the
// DMIJ and DMIJI are matrix names, not references. The cards are kept whole next to a column of their UXIDs, which is
// what the query engine and the Arrow export see.
class AeroDownwashTable final : public TypedTable<AeroDownwashTable, AeroDownwashCard>
{
public:
    static constexpr std::string_view NAME = "AeroDownwash";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroDownwashFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroDownwashCard&) noexcept { return 0; }
    std::uint32_t append(const AeroDownwashCard& card, base::CardKey key);
    [[nodiscard]] AeroDownwashCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroDownwashCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& controlVectorIdColumn() noexcept { return m_controlVectorId; }

private:
    Column<base::EntityId> m_controlVectorId;
    CardRows<AeroDownwashCard> m_cards;
};

} // namespace oor::model
