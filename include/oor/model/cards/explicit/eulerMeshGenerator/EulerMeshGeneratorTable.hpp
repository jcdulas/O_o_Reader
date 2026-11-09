#pragma once

#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerMeshGeneratorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerMeshGeneratorCard& card);
    [[nodiscard]] static std::optional<EulerMeshGeneratorCard> read(FieldReader& reader);
    static void write(const EulerMeshGeneratorCard& card, FieldWriter& writer);

    // Gives the fields whose default depends on another field the value of that field (X0, Y0, Z0 of a BOX mesh from
    // the reference point; TID-Y, TID-Z from TID-X; NBY, NBZ from NBX; NPY, NPZ from NPX), where they are blank. The
    // reader and the Python constructor both call it, so a card built in memory is the card the text reads back as.
    static void resolveDefaults(EulerMeshGeneratorCard& card);
};

// All the MESH cards. The MID is unique among the MESH cards (IdSpace::PerCardType) and is in the category EulerMesh,
// which a COUPLE, a BARRIER or a flow boundary names. A MESH names a property (PEULER or PEULER1), another mesh
// (SUBMESH), tables of resizing, a coordinate system and the BIAS cards of the three directions.
class EulerMeshGeneratorTable final : public TypedTable<EulerMeshGeneratorTable, EulerMeshGeneratorCard>
{
public:
    static constexpr std::string_view NAME = "EulerMeshGenerator";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerMesh, base::IdSpace::PerCardType};
    using Format = EulerMeshGeneratorFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerMeshGeneratorCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerMeshGeneratorCard& card, base::CardKey key);
    [[nodiscard]] EulerMeshGeneratorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerMeshGeneratorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerMeshGeneratorCard> m_cards;
};

} // namespace oor::model
