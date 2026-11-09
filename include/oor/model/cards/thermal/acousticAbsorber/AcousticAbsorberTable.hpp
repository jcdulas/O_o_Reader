#pragma once

#include "oor/model/cards/thermal/acousticAbsorber/AcousticAbsorberCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticAbsorberFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticAbsorberCard& card);
    [[nodiscard]] static std::optional<AcousticAbsorberCard> read(FieldReader& reader);
    static void write(const AcousticAbsorberCard& card, FieldWriter& writer);
};

// All the CAABSF cards. The ID is an element ID, in the space every element shares (IdSpace::Element); PID names a
// PAABSF.
class AcousticAbsorberTable final : public TypedTable<AcousticAbsorberTable, AcousticAbsorberCard>
{
public:
    static constexpr std::string_view NAME = "AcousticAbsorber";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticAbsorberFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticAbsorberCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticAbsorberCard& card, base::CardKey key);
    [[nodiscard]] AcousticAbsorberCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticAbsorberCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AcousticAbsorberCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AcousticAbsorberCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
