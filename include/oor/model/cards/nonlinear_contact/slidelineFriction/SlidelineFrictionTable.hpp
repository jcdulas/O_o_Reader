#pragma once

#include "oor/model/cards/nonlinear_contact/slidelineFriction/SlidelineFrictionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SlidelineFrictionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SlidelineFrictionCard& card);
    [[nodiscard]] static std::optional<SlidelineFrictionCard> read(FieldReader& reader);
    static void write(const SlidelineFrictionCard& card, FieldWriter& writer);
};

// All the BFRIC cards. The ID is unique among them (IdSpace::PerCardType) and is what the BCONP FRICID field names
// (category SlidelineFriction).
class SlidelineFrictionTable final : public TypedTable<SlidelineFrictionTable, SlidelineFrictionCard>
{
public:
    static constexpr std::string_view NAME = "SlidelineFriction";
    static inline const CardTypeInfo INFO{NAME, CardCategory::SlidelineFriction, base::IdSpace::PerCardType};
    using Format = SlidelineFrictionFormat;

    [[nodiscard]] static base::EntityId cardId(const SlidelineFrictionCard& card) noexcept { return card.id; }
    std::uint32_t append(const SlidelineFrictionCard& card, base::CardKey key);
    [[nodiscard]] SlidelineFrictionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SlidelineFrictionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no reference

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& stickStiffnessColumn() noexcept { return m_stickStiffness; }
    [[nodiscard]] Column<double>& staticFrictionColumn() noexcept { return m_staticFriction; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_stickStiffness;
    Column<double> m_staticFriction;
};

} // namespace oor::model
