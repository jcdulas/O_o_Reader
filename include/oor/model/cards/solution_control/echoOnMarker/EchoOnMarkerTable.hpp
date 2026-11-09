#pragma once

#include "oor/model/cards/solution_control/echoOnMarker/EchoOnMarkerCard.hpp"
#include "oor/model/core/TypedTable.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EchoOnMarkerFormat
{
    // An ECHOON has no value that can be wrong: nothing to check.
    [[nodiscard]] static std::optional<CardProblem> check(const EchoOnMarkerCard& card);
    [[nodiscard]] static std::optional<EchoOnMarkerCard> read(FieldReader& reader);
    static void write(const EchoOnMarkerCard& card, FieldWriter& writer);
};

// All the ECHOON cards: a row each and nothing else, since the card has no field and no ID (IdSpace::None).
class EchoOnMarkerTable final : public TypedTable<EchoOnMarkerTable, EchoOnMarkerCard>
{
public:
    static constexpr std::string_view NAME = "EchoOnMarker";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = EchoOnMarkerFormat;

    [[nodiscard]] static base::EntityId cardId(const EchoOnMarkerCard&) noexcept { return 0; }
    std::uint32_t append(const EchoOnMarkerCard&, base::CardKey key) { return m_rows.append(key); }
    [[nodiscard]] EchoOnMarkerCard card(std::uint32_t) const { return {}; }
    void store(std::uint32_t, const EchoOnMarkerCard&) {}
    void keepRows(const std::vector<std::uint32_t>&) {}

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field, no reference
};

} // namespace oor::model
