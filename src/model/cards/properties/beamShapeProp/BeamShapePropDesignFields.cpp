#include "oor/model/cards/properties/beamShapeProp/BeamShapePropDesignFields.hpp"

#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

namespace oor::model
{

namespace
{

enum BeamShapePropKind : std::int32_t
{
    Dimension, // `index` is the number of the dimension minus one
    NonstructuralMass,
};

// EndA is the first station; EndB is the station at ratio 1. Numeric positions include EndA as position 1.
std::optional<std::size_t> stationOf(const BeamShapePropCard& card, const StationName& station)
{
    const std::size_t count = card.stations.size();
    if (!station.hasStation || station.atEndA)
        return 0;
    if (station.atEndB)
        return count > 1 && card.stations.back().stationRatio == 1.0 ? std::optional<std::size_t>(count - 1)
                                                                     : std::nullopt;
    const std::size_t index = static_cast<std::size_t>(station.intermediate - 1);
    return index < count ? std::optional<std::size_t>(index) : std::nullopt;
}

DesignFieldLookup findField(const BeamShapePropCard& card, const DesignField& field)
{
    constexpr const char* DESIGNABLE =
        "dimension(1), dimension(2), ... and nonStructMass, each with an optional station (EndA), (EndB) or (i)";
    const bool dimensionOnly = field.name.starts_with("dimension(") && field.name.find(')') == field.name.size() - 1;
    const std::string selected = dimensionOnly ? field.name + "(EndA)" : field.name;
    const std::optional<StationName> parsed = splitStation(selected);
    if (!parsed)
        return DesignFieldLookup::refused(refusalOfField("BeamShapeProp", field, DESIGNABLE));
    const std::optional<std::size_t> station = stationOf(card, *parsed);
    std::string stationText = "EndA";
    if (parsed->hasStation && !parsed->atEndA)
        stationText = parsed->atEndB ? "EndB" : std::to_string(parsed->intermediate);
    int index = -1;
    bool isNsm = false;
    if (parsed->word == "nonStructMass")
    {
        isNsm = true;
    }
    else
    {
        const NumberedName named = splitTrailingNumber(parsed->word);
        if (named.word != "dimension" || named.number < 1)
            return DesignFieldLookup::refused(refusalOfField("BeamShapeProp", field, DESIGNABLE));
        index = named.number - 1;
    }
    if (!station)
        return DesignFieldLookup::refused(field.name + ": this BeamShapeProp has " +
                                          std::to_string(card.stations.size()) +
                                          " stations and none matches the station asked for");
    const BeamShapePropStation& chosen = card.stations[*station];
    if (!isNsm && static_cast<std::size_t>(index) >= chosen.dims.size())
        return DesignFieldLookup::refused("a BeamShapeProp of type " + card.kind + " has " +
                                          std::to_string(chosen.dims.size()) + " dimensions: " + field.name +
                                          " is not one of them");
    if (isNsm)
        return DesignFieldLookup::found(designFacts(NonstructuralMass, "nonStructMass(" + stationText + ")",
                                                    DesignLowerLimit::Positive, DesignMassEffect::Linear, 0,
                                                    static_cast<std::int32_t>(*station)));
    return DesignFieldLookup::found(designFacts(
        Dimension, "dimension(" + std::to_string(index + 1) + ")" + "(" + stationText + ")", DesignLowerLimit::Positive,
        DesignMassEffect::Nonlinear, index, static_cast<std::int32_t>(*station)));
}

double readField(const BeamShapePropCard& card, const DesignFieldFacts& facts)
{
    const BeamShapePropStation& station = card.stations[static_cast<std::size_t>(facts.station)];
    return facts.kind == Dimension ? station.dims[static_cast<std::size_t>(facts.index)] : station.nonStructMass;
}

void writeFields(BeamShapePropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        BeamShapePropStation& station = card.stations[static_cast<std::size_t>(item.facts->station)];
        if (item.facts->kind == Dimension)
            station.dims[static_cast<std::size_t>(item.facts->index)] = item.value;
        else
            station.nonStructMass = item.value;
    }
}

// The contract columns: station_dims (the dimensions of every station, one station after the other) and station_nsm
// (one value per station), in the order of the stations of the card.
DesignColumn columnOf(const BeamShapePropCard& card, const DesignFieldFacts& facts)
{
    if (facts.kind != Dimension)
        return {"stationNsm", facts.station};
    std::int32_t before = 0;
    for (std::int32_t station = 0; station < facts.station; ++station)
        before += static_cast<std::int32_t>(card.stations[static_cast<std::size_t>(station)].dims.size());
    return {"stationDims", before + facts.index};
}

} // namespace

const DesignableCard& beamShapePropDesignFields()
{
    static const TableDesignFields<BeamShapePropTable> FIELDS("BeamShapeProp",
                                                              {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
