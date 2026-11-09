#pragma once

#include "oor/model/cards/materials/isoMatFreq/IsoMatFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IsoMatFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IsoMatFreqCard& card);
    [[nodiscard]] static std::optional<IsoMatFreqCard> read(FieldReader& reader);
    static void write(const IsoMatFreqCard& card, FieldWriter& writer);
};

// All the MAT1F cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEDi cards (ReferenceTarget::DynamicTable).
class IsoMatFreqTable final : public TypedTable<IsoMatFreqTable, IsoMatFreqCard>
{
public:
    static constexpr std::string_view NAME = "IsoMatFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = IsoMatFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const IsoMatFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const IsoMatFreqCard& card, base::CardKey key);
    [[nodiscard]] IsoMatFreqCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const IsoMatFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& youngsModulusTableIdColumn() noexcept { return m_youngsModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulusTableIdColumn() noexcept { return m_shearModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatioTableIdColumn() noexcept { return m_poissonRatioTableId; }
    [[nodiscard]] Column<base::EntityId>& structuralDampingTableIdColumn() noexcept
    {
        return m_structuralDampingTableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_youngsModulusTableId;
    Column<base::EntityId> m_shearModulusTableId;
    Column<base::EntityId> m_poissonRatioTableId;
    Column<base::EntityId> m_structuralDampingTableId;
};

} // namespace oor::model
