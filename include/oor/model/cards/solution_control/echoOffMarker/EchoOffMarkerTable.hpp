#pragma once

#include "oor/model/cards/solution_control/echoOffMarker/EchoOffMarkerCard.hpp"
#include "oor/model/core/TypedTable.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EchoOffMarkerFormat
{
    // An ECHOOFF has no value that can be wrong: nothing to check.
    [[nodiscard]] static std::optional<CardProblem> check(const EchoOffMarkerCard& card);
    [[nodiscard]] static std::optional<EchoOffMarkerCard> read(FieldReader& reader);
    static void write(const EchoOffMarkerCard& card, FieldWriter& writer);
};

// All the ECHOOFF cards: a row each and nothing else, since the card has no field and no ID (IdSpace::None).
class EchoOffMarkerTable final : public TypedTable<EchoOffMarkerTable, EchoOffMarkerCard>
{
public:
    static constexpr std::string_view NAME = "EchoOffMarker";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = EchoOffMarkerFormat;

    [[nodiscard]] static base::EntityId cardId(const EchoOffMarkerCard&) noexcept { return 0; }
    std::uint32_t append(const EchoOffMarkerCard&, base::CardKey key) { return m_rows.append(key); }
    [[nodiscard]] EchoOffMarkerCard card(std::uint32_t) const { return {}; }
    void store(std::uint32_t, const EchoOffMarkerCard&) {}
    void keepRows(const std::vector<std::uint32_t>&) {}

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field, no reference
};

} // namespace oor::model
