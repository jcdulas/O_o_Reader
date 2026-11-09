#include "oor/model/cards/solution_control/echoOnMarker/EchoOnMarkerTable.hpp"

namespace oor::model
{

std::optional<EchoOnMarkerCard> EchoOnMarkerFormat::read(FieldReader& reader)
{
    if (reader.lastUsedField() > 0)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has no field", "");
    if (reader.failed())
        return std::nullopt;
    return EchoOnMarkerCard{};
}

std::optional<CardProblem> EchoOnMarkerFormat::check(const EchoOnMarkerCard&)
{
    return std::nullopt;
}

void EchoOnMarkerFormat::write(const EchoOnMarkerCard&, FieldWriter&) {}

} // namespace oor::model
