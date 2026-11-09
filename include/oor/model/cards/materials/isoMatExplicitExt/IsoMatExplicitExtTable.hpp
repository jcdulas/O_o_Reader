#pragma once

#include "oor/model/cards/materials/isoMatExplicitExt/IsoMatExplicitExtCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IsoMatExplicitExtFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IsoMatExplicitExtCard& card);
    [[nodiscard]] static std::optional<IsoMatExplicitExtCard> read(FieldReader& reader);
    static void write(const IsoMatExplicitExtCard& card, FieldWriter& writer);
};

// All the MAT1A cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it). EID names an equation of state card,
// which no card types yet, so it is a plain ID that the reference checks leave alone.
class IsoMatExplicitExtTable final : public TypedTable<IsoMatExplicitExtTable, IsoMatExplicitExtCard>
{
public:
    static constexpr std::string_view NAME = "IsoMatExplicitExt";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = IsoMatExplicitExtFormat;

    [[nodiscard]] static base::EntityId cardId(const IsoMatExplicitExtCard& card) noexcept { return card.id; }
    std::uint32_t append(const IsoMatExplicitExtCard& card, base::CardKey key);
    [[nodiscard]] IsoMatExplicitExtCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const IsoMatExplicitExtCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& equationOfStateIdColumn() noexcept { return m_equationOfStateId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_equationOfStateId;
};

} // namespace oor::model
