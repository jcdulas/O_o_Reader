#pragma once

#include "oor/model/cards/superelements/superelementGeneralizedPointCount/SuperelementGeneralizedPointCountCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementGeneralizedPointCountFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementGeneralizedPointCountCard& card);
    [[nodiscard]] static std::optional<SuperelementGeneralizedPointCountCard> read(FieldReader& reader);
    static void write(const SuperelementGeneralizedPointCountCard& card, FieldWriter& writer);
};

// All the SENQSET cards, category Superelement, no ID of their own (IdSpace::None). A column holds the SEID, which is
// -1 for ALL, next to the count.
class SuperelementGeneralizedPointCountTable final
    : public TypedTable<SuperelementGeneralizedPointCountTable, SuperelementGeneralizedPointCountCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementGeneralizedPointCount";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    // What the SEID column holds for the word ALL.
    static constexpr base::EntityId ALL_IN_COLUMN = -1;
    using Format = SuperelementGeneralizedPointCountFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementGeneralizedPointCountCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementGeneralizedPointCountCard& card, base::CardKey key);
    [[nodiscard]] SuperelementGeneralizedPointCountCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementGeneralizedPointCountCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<base::EntityId>& countColumn() noexcept { return m_count; }

private:
    Column<base::EntityId> m_superelementId;
    Column<base::EntityId> m_count;
};

} // namespace oor::model
