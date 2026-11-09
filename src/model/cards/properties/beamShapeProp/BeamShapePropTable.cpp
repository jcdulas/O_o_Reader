#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/properties/sections/SectionCatalogue.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int FIRST_DIMENSION_FIELD = 9;
constexpr double NOT_GIVEN = std::numeric_limits<double>::quiet_NaN(); // a blank value, until resolved

const char* dimensionName(std::size_t index)
{
    static constexpr const char* NAMES[10] = {"dimension1", "dimension2", "dimension3", "dimension4", "dimension5",
                                              "dimension6", "dimension7", "dimension8", "dimension9", "dimension10"};
    return NAMES[index < 10 ? index : 9];
}

// The first station after end A that is at X/XB = 1.0 (end B); stations.size() when there is none.
std::size_t endBIndex(const std::vector<BeamShapePropStation>& stations)
{
    for (std::size_t index = 1; index < stations.size(); ++index)
    {
        if (stations[index].stationRatio == 1.0)
            return index;
    }
    return stations.size();
}

// A DBOX station that leaves DIM5 to DIM10 blank: DIM5 to DIM8 are DIM4, DIM9 and DIM10 are DIM6.
void applyDboxDefaults(BeamShapePropStation& station)
{
    if (station.dims.size() != 10)
        return;
    if (!std::isnan(station.dims[3]))
    {
        for (std::size_t index = 4; index < 8; ++index)
        {
            if (std::isnan(station.dims[index]))
                station.dims[index] = station.dims[3];
        }
    }
    if (!std::isnan(station.dims[5]))
    {
        for (std::size_t index = 8; index < 10; ++index)
        {
            if (std::isnan(station.dims[index]))
                station.dims[index] = station.dims[5];
        }
    }
}

} // namespace

bool BeamShapePropFormat::resolveBlankValues(std::vector<BeamShapePropStation>& stations)
{
    if (stations.size() < 2)
        return true;
    const BeamShapePropStation& endA = stations.front();
    const std::size_t endB = endBIndex(stations);
    const auto resolve = [&](double atA, double& atB, double& value, double stationRatio)
    {
        if (std::isnan(atB))
            atB = atA;
        if (std::isnan(value))
            value = atA + (atB - atA) * stationRatio;
    };
    for (std::size_t index = 1; index < stations.size(); ++index)
    {
        BeamShapePropStation& station = stations[index];
        bool blank = std::isnan(station.nonStructMass);
        for (const double dim : station.dims)
            blank = blank || std::isnan(dim);
        if (blank && endB == stations.size())
            return false;
        if (!blank)
            continue;
        // End B first: an intermediate station interpolates between the two ends.
        BeamShapePropStation& end = stations[endB];
        for (std::size_t dim = 0; dim < station.dims.size(); ++dim)
            resolve(endA.dims[dim], end.dims[dim], station.dims[dim], station.stationRatio);
        resolve(endA.nonStructMass, end.nonStructMass, station.nonStructMass, station.stationRatio);
    }
    return true;
}

std::optional<BeamShapePropCard> BeamShapePropFormat::read(FieldReader& reader)
{
    BeamShapePropCard card;
    card.stations.clear();
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    if (const std::string_view group = reader.text(3); !group.empty() && !(group == sections::STANDARD_SECTION_LIBRARY))
        reader.error("card.bad-value",
                     "only the standard section library (" + std::string("sectionGroup") +
                         " blank or its name) is supported, found \"" + std::string(group) + "\"",
                     "sectionGroup");
    card.kind = std::string(reader.text(4));
    if (card.kind.empty())
    {
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
        return card;
    }
    const std::size_t count = sections::dimensionCount(card.kind);
    if (count == 0)
    {
        reader.error("card.bad-value",
                     std::string("kind") + " \"" + card.kind + "\" is not a section of the standard library", "kind");
        return card;
    }
    for (int field = 5; field <= 8; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 5 to 8 of a " + std::string(reader.cardName()) + " must be blank",
                         "kind");
    }

    // End A: the dimensions, then NSM. The fields after it are the stations that follow, until the last one used.
    BeamShapePropStation endA;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto value = reader.optionalReal(FIRST_DIMENSION_FIELD + static_cast<int>(index), dimensionName(index));
        endA.dims.push_back(value.value_or(NOT_GIVEN));
    }
    if (card.kind == "DoubleBoxSection")
        applyDboxDefaults(endA);
    for (std::size_t index = 0; index < count; ++index)
    {
        if (std::isnan(endA.dims[index]))
            reader.error("card.missing-field", std::string(dimensionName(index)) + " is required at end A",
                         dimensionName(index));
    }
    endA.nonStructMass = reader.real(FIRST_DIMENSION_FIELD + static_cast<int>(count), "nonStructMass", 0.0);
    card.stations.push_back(endA);

    const int blockWidth = static_cast<int>(count) + 3; // SO, X/XB, the dimensions, NSM
    int field = FIRST_DIMENSION_FIELD + static_cast<int>(count) + 1;
    while (field <= reader.lastUsedField())
    {
        BeamShapePropStation station;
        const std::string_view stressOutput = reader.text(field);
        if (stressOutput.empty() || (stressOutput == "Yes"))
            station.stressOutput = true;
        else if ((stressOutput == "No"))
            station.stressOutput = false;
        else
            reader.error(
                "card.bad-value",
                std::string("stressOutput") + " must be YES or NO, found \"" + std::string(stressOutput) + "\"",
                "stressOutput");
        station.stationRatio = reader.real(field + 1, "stationRatio", 1.0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto value = reader.optionalReal(field + 2 + static_cast<int>(index), dimensionName(index));
            station.dims.push_back(value.value_or(NOT_GIVEN));
        }
        station.nonStructMass =
            reader.optionalReal(field + 2 + static_cast<int>(count), "nonStructMass").value_or(NOT_GIVEN);
        if (card.kind == "DoubleBoxSection")
            applyDboxDefaults(station);
        card.stations.push_back(std::move(station));
        field += blockWidth;
    }

    if (reader.failed())
        return card;
    if (!resolveBlankValues(card.stations))
    {
        reader.error(
            "card.missing-field",
            "a station leaves a value blank but no station is at " + std::string("stationRatio") + " = 1.0 (end B)",
            "stationRatio");
        return card;
    }
    reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamShapePropFormat::check(const BeamShapePropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    const std::size_t count = sections::dimensionCount(card.kind);
    if (count == 0)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be a section of the standard library, not \"" + card.kind + "\"",
            "kind"};
    if (card.stations.empty())
        return CardProblem{"card.missing-field", "the card needs at least end A", "dimension1"};
    if (card.stations.size() * count > BeamShapePropCard::MAX_TOTAL_DIMENSIONS)
        return CardProblem{"card.bad-value", "the total number of dimensions at all stations must be less than 200",
                           "dimension1"};
    const BeamShapePropStation& endA = card.stations.front();
    if (endA.stationRatio != 0.0)
        return CardProblem{"card.bad-value", "end A is at " + std::string(CardProblem::FIELD) + " = 0.0",
                           "stationRatio"};
    if (!endA.stressOutput)
        return CardProblem{"card.bad-value",
                           "end A has no " + std::string(CardProblem::FIELD) + " field: its output is always requested",
                           "stressOutput"};
    for (std::size_t index = 0; index < card.stations.size(); ++index)
    {
        const BeamShapePropStation& station = card.stations[index];
        if (station.dims.size() != count)
            return CardProblem{"card.bad-value",
                               "a " + card.kind + " section takes " + std::to_string(count) + " dimensions, not " +
                                   std::to_string(station.dims.size()),
                               "dimension1"};
        if (index > 0)
        {
            if (!(station.stationRatio > 0.0 && station.stationRatio <= 1.0))
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " must be greater than 0.0 and at most 1.0",
                                   "stationRatio"};
            for (std::size_t other = 0; other < index; ++other)
            {
                if (card.stations[other].stationRatio == station.stationRatio)
                    return CardProblem{"card.bad-value",
                                       "the stations must have different " + std::string(CardProblem::FIELD),
                                       "stationRatio"};
            }
        }
        for (std::size_t dim = 0; dim < count; ++dim)
        {
            if (!(station.dims[dim] > 0.0))
                return CardProblem{"card.bad-value", "the dimensions must be greater than 0.0", dimensionName(dim)};
        }
        if (std::isnan(station.nonStructMass))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a number",
                               "nonStructMass"};
    }
    return std::nullopt;
}

void BeamShapePropFormat::write(const BeamShapePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    // GROUP is blank: the standard library is the default and the only one supported.
    writer.text(4, card.kind);
    const BeamShapePropStation& endA = card.stations.front();
    int field = FIRST_DIMENSION_FIELD;
    for (const double dim : endA.dims)
        writer.real(field++, dim);
    writer.realUnlessDefault(field, endA.nonStructMass, 0.0);
    ++field;
    for (std::size_t index = 1; index < card.stations.size(); ++index)
    {
        const BeamShapePropStation& station = card.stations[index];
        writer.textUnlessDefault(field, station.stressOutput ? "Yes" : "No", "Yes");
        // What a station leaves blank is interpolated from the two ends: every value is written, so that the station
        // does not move with them.
        writer.real(field + 1, station.stationRatio);
        field += 2;
        for (const double dim : station.dims)
            writer.real(field++, dim);
        writer.real(field++, station.nonStructMass);
    }
}

namespace
{

// The lists of a card, as the table stores them.
struct StationLists
{
    std::vector<double> position;
    std::vector<std::uint8_t> output;
    std::vector<double> nonStructMass;
    std::vector<double> dims;
};

StationLists listsOf(const std::vector<BeamShapePropStation>& stations)
{
    StationLists lists;
    for (const BeamShapePropStation& station : stations)
    {
        lists.position.push_back(station.stationRatio);
        lists.output.push_back(station.stressOutput ? 1 : 0);
        lists.nonStructMass.push_back(station.nonStructMass);
        lists.dims.insert(lists.dims.end(), station.dims.begin(), station.dims.end());
    }
    return lists;
}

} // namespace

std::uint32_t BeamShapePropTable::append(const BeamShapePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_kind.push(card.kind);
    const StationLists lists = listsOf(card.stations);
    m_stationRatio.push(lists.position);
    m_stationOutput.push(lists.output);
    m_stationNonStructMass.push(lists.nonStructMass);
    m_stationDims.push(lists.dims);
    return row;
}

BeamShapePropCard BeamShapePropTable::card(std::uint32_t row) const
{
    BeamShapePropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.kind = m_kind.get(row);
    const auto position = m_stationRatio.get(row);
    const auto output = m_stationOutput.get(row);
    const auto nsm = m_stationNonStructMass.get(row);
    const auto dims = m_stationDims.get(row);
    const std::size_t perStation = position.empty() ? 0 : dims.size() / position.size();
    card.stations.clear();
    for (std::size_t index = 0; index < position.size(); ++index)
    {
        BeamShapePropStation station;
        station.stationRatio = position[index];
        station.stressOutput = output[index] != 0;
        station.nonStructMass = nsm[index];
        station.dims.assign(dims.begin() + static_cast<std::ptrdiff_t>(index * perStation),
                            dims.begin() + static_cast<std::ptrdiff_t>((index + 1) * perStation));
        card.stations.push_back(std::move(station));
    }
    return card;
}

void BeamShapePropTable::store(std::uint32_t row, const BeamShapePropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_kind.set(row, card.kind);
    const StationLists lists = listsOf(card.stations);
    m_stationRatio.set(row, lists.position);
    m_stationOutput.set(row, lists.output);
    m_stationNonStructMass.set(row, lists.nonStructMass);
    m_stationDims.set(row, lists.dims);
}

void BeamShapePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_kind.keepRows(rows);
    m_stationRatio.keepRows(rows);
    m_stationOutput.keepRows(rows);
    m_stationNonStructMass.keepRows(rows);
    m_stationDims.keepRows(rows);
}

void BeamShapePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
