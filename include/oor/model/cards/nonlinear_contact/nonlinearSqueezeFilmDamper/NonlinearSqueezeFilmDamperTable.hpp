#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearSqueezeFilmDamper/NonlinearSqueezeFilmDamperCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearSqueezeFilmDamperFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearSqueezeFilmDamperCard& card);
    [[nodiscard]] static std::optional<NonlinearSqueezeFilmDamperCard> read(FieldReader& reader);
    static void write(const NonlinearSqueezeFilmDamperCard& card, FieldWriter& writer);
};

// All the NLRSFD cards. The SID is a nonlinear load set ID shared by the NOLIN cards and the other nonlinear loads
// (IdSpace::Group, category DynamicLoad): the NONLINEAR Case Control command selects it. GA and GB are grid points. The
// cards are kept whole (CardRows); the query engine reads the SID only.
class NonlinearSqueezeFilmDamperTable final
    : public TypedTable<NonlinearSqueezeFilmDamperTable, NonlinearSqueezeFilmDamperCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearSqueezeFilmDamper";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = NonlinearSqueezeFilmDamperFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearSqueezeFilmDamperCard& card) noexcept
    {
        return card.setId;
    }
    std::uint32_t append(const NonlinearSqueezeFilmDamperCard& card, base::CardKey key);
    [[nodiscard]] NonlinearSqueezeFilmDamperCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearSqueezeFilmDamperCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<NonlinearSqueezeFilmDamperCard> m_cards;
};

} // namespace oor::model
