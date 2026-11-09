#pragma once

#include "oor/model/cards/aero/aeroGust/AeroGustCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroGustFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroGustCard& card);
    [[nodiscard]] static std::optional<AeroGustCard> read(FieldReader& reader);
    static void write(const AeroGustCard& card, FieldWriter& writer);
};

// All the GUST cards, in the category AeroGustSet. The SID is unique among the GUST cards (IdSpace::PerCardType) and is
// what the Case Control GUST names (ReferenceTarget::AeroGustSet). DLOAD is the set ID of a TLOADi or RLOADi
// (ReferenceTarget::DynamicLoadSet).
class AeroGustTable final : public TypedTable<AeroGustTable, AeroGustCard>
{
public:
    static constexpr std::string_view NAME = "AeroGust";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroGustSet, base::IdSpace::PerCardType};
    using Format = AeroGustFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroGustCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroGustCard& card, base::CardKey key);
    [[nodiscard]] AeroGustCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroGustCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& dynamicLoadIdColumn() noexcept { return m_dynamicLoadId; }
    [[nodiscard]] Column<double>& gustScaleColumn() noexcept { return m_gustScale; }
    [[nodiscard]] Column<double>& gustOriginXColumn() noexcept { return m_gustOriginX; }
    [[nodiscard]] Column<double>& velocityColumn() noexcept { return m_velocity; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_dynamicLoadId;
    Column<double> m_gustScale;
    Column<double> m_gustOriginX;
    Column<double> m_velocity;
};

} // namespace oor::model
