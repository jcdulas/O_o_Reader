#pragma once

#include "oor/model/cards/aero/aeroPressureVector/AeroPressureVectorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroPressureVectorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroPressureVectorCard& card);
    [[nodiscard]] static std::optional<AeroPressureVectorCard> read(FieldReader& reader);
    static void write(const AeroPressureVectorCard& card, FieldWriter& writer);
};

// All the AEPRESS cards. An AEPRESS has no numeric ID of its own (IdSpace::None, category Aero): there is one for each
// Mach number, symmetry and control vector, so a deck has many. The UXID names a UXVEC
// (ReferenceTarget::ControlVector); the DMIJ and DMIJI are matrix names, not references. The cards are kept whole next
// to a column of their UXIDs, which is what the query engine and the Arrow export see.
class AeroPressureVectorTable final : public TypedTable<AeroPressureVectorTable, AeroPressureVectorCard>
{
public:
    static constexpr std::string_view NAME = "AeroPressureVector";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroPressureVectorFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroPressureVectorCard&) noexcept { return 0; }
    std::uint32_t append(const AeroPressureVectorCard& card, base::CardKey key);
    [[nodiscard]] AeroPressureVectorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroPressureVectorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& controlVectorIdColumn() noexcept { return m_controlVectorId; }

private:
    Column<base::EntityId> m_controlVectorId;
    CardRows<AeroPressureVectorCard> m_cards;
};

} // namespace oor::model
