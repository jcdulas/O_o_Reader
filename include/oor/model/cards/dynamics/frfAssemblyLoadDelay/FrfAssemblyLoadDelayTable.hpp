#pragma once

#include "oor/model/cards/dynamics/frfAssemblyLoadDelay/FrfAssemblyLoadDelayCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FrfAssemblyLoadDelayFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfAssemblyLoadDelayCard& card);
    [[nodiscard]] static std::optional<FrfAssemblyLoadDelayCard> read(FieldReader& reader);
    static void write(const FrfAssemblyLoadDelayCard& card, FieldWriter& writer);
};

// All the FBADLAY cards. The SID is a set ID shared by many cards, in the category of the DELAY cards (IdSpace::Group,
// category Delay), so the DELAY field of a dynamic load finds it (ReferenceTarget::DelaySet). A component given by its
// ID is a reference to an FRFCOMP (ReferenceTarget::FrfComponent); one given by its name is kept as written; the point
// is a plain number of the component's own model.
class FrfAssemblyLoadDelayTable final : public TypedTable<FrfAssemblyLoadDelayTable, FrfAssemblyLoadDelayCard>
{
public:
    static constexpr std::string_view NAME = "FrfAssemblyLoadDelay";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Delay, base::IdSpace::Group};
    using Format = FrfAssemblyLoadDelayFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfAssemblyLoadDelayCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfAssemblyLoadDelayCard& card, base::CardKey key);
    [[nodiscard]] FrfAssemblyLoadDelayCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FrfAssemblyLoadDelayCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& frfComponentIdColumn() noexcept { return m_frfComponentId; }
    [[nodiscard]] Column<std::string>& frfComponentNameColumn() noexcept { return m_frfComponentName; }
    [[nodiscard]] Column<base::EntityId>& pointIdColumn() noexcept { return m_pointId; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<double>& delayColumn() noexcept { return m_delay; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_frfComponentId; // 0 when the card names the component by its name
    Column<std::string> m_frfComponentName;  // empty when the card names the component by its ID
    Column<base::EntityId> m_pointId;
    Column<std::int32_t> m_components;
    Column<double> m_delay;
};

} // namespace oor::model
