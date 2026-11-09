#include "oor/model/cards/solution_control/echoOffMarker/EchoOffMarkerTable.hpp"

namespace oor::model
{

std::optional<EchoOffMarkerCard> EchoOffMarkerFormat::read(FieldReader& reader)
{
    if (reader.lastUsedField() > 0)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has no field", "");
    if (reader.failed())
        return std::nullopt;
    return EchoOffMarkerCard{};
}

std::optional<CardProblem> EchoOffMarkerFormat::check(const EchoOffMarkerCard&)
{
    return std::nullopt;
}

void EchoOffMarkerFormat::write(const EchoOffMarkerCard&, FieldWriter&) {}

} // namespace oor::model
