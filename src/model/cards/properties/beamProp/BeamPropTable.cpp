#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int FIRST_STATION_FIELD = 9;
constexpr int FIELDS_PER_LINE = 8;
constexpr double NOT_GIVEN = std::numeric_limits<double>::quiet_NaN(); // a blank section property, until resolved

// The six section properties of a station, in the order of the card.
constexpr double BeamPropStation::* SECTION_MEMBERS[6] = {&BeamPropStation::area,
                                                          &BeamPropStation::bendingInertia1,
                                                          &BeamPropStation::bendingInertia2,
                                                          &BeamPropStation::productInertia12,
                                                          &BeamPropStation::torsionConstant,
                                                          &BeamPropStation::nonStructMass};

bool startsWithLetter(std::string_view text) noexcept
{
    if (text.empty())
        return false;
    const char first = base::toUpperAscii(text.front());
    return first >= 'A' && first <= 'Z';
}

const char* outputName(BeamStressOutput output)
{
    switch (output)
    {
    case BeamStressOutput::Yes:
        return "Yes";
    case BeamStressOutput::YesA:
        return "YesSameAsEndA";
    case BeamStressOutput::No:
        break;
    }
    return "No";
}

// The line of stress points C, D, E, F starting at `field`: blank is 0.0.
void readStressPoints(FieldReader& reader, int field, BeamPropStation& station)
{
    static constexpr const char* NAMES[8] = {"stressPointCy", "stressPointCz", "stressPointDy", "stressPointDz",
                                             "stressPointEy", "stressPointEz", "stressPointFy", "stressPointFz"};
    std::array<double, 2>* const points[4] = {&station.stressPointC, &station.stressPointD, &station.stressPointE,
                                              &station.stressPointF};
    for (int index = 0; index < 8; ++index)
        (*points[index / 2])[static_cast<std::size_t>(index % 2)] = reader.real(field + index, NAMES[index], 0.0);
}

void writeStressPoints(FieldWriter& writer, int field, const BeamPropStation& station)
{
    const std::array<double, 2>* const points[4] = {&station.stressPointC, &station.stressPointD, &station.stressPointE,
                                                    &station.stressPointF};
    for (int index = 0; index < 8; ++index)
        writer.realUnlessDefault(field + index, (*points[index / 2])[static_cast<std::size_t>(index % 2)], 0.0);
}

// The first station after end A that is at X/XB = 1.0 (end B); stations.size() when there is none.
std::size_t endBIndex(const std::vector<BeamPropStation>& stations)
{
    for (std::size_t index = 1; index < stations.size(); ++index)
    {
        if (stations[index].stationRatio == 1.0)
            return index;
    }
    return stations.size();
}

} // namespace

bool BeamPropFormat::resolveBlankSections(std::vector<BeamPropStation>& stations)
{
    if (stations.size() < 2)
        return true;
    const std::size_t endB = endBIndex(stations);
    if (endB == stations.size())
        return false;
    for (double BeamPropStation::* member : SECTION_MEMBERS)
    {
        const double atA = stations[0].*member;
        double& atB = stations[endB].*member;
        if (std::isnan(atB))
            atB = atA;
        for (std::size_t index = 1; index < stations.size(); ++index)
        {
            double& value = stations[index].*member;
            if (std::isnan(value))
                value = atA + (atB - atA) * stations[index].stationRatio;
        }
    }
    return true;
}

std::optional<BeamPropCard> BeamPropFormat::read(FieldReader& reader)
{
    static constexpr const char* SECTION_NAMES[6] = {
        "area", "bendingInertia1", "bendingInertia2", "productInertia12", "torsionConstant", "nonStructMass"};

    BeamPropCard card;
    card.stations.clear();
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");

    // End A: the section properties, then the line of stress points unless the next field is the SO of a station.
    BeamPropStation endA;
    endA.area = reader.requiredReal(3, SECTION_NAMES[0]);
    endA.bendingInertia1 = reader.requiredReal(4, SECTION_NAMES[1]);
    endA.bendingInertia2 = reader.requiredReal(5, SECTION_NAMES[2]);
    endA.productInertia12 = reader.real(6, SECTION_NAMES[3], 0.0);
    endA.torsionConstant = reader.real(7, SECTION_NAMES[4], 0.0);
    endA.nonStructMass = reader.real(8, SECTION_NAMES[5], 0.0);
    int field = FIRST_STATION_FIELD;
    if (startsWithLetter(reader.text(field)))
    {
        endA.stressOutput = BeamStressOutput::No;
    }
    else
    {
        readStressPoints(reader, field, endA);
        field += FIELDS_PER_LINE;
    }
    card.stations.push_back(endA);

    // The further stations: a word (SO) starts each one, a number or a blank starts the last two lines.
    while (startsWithLetter(reader.text(field)))
    {
        if (card.stations.size() == BeamPropCard::MAX_STATIONS)
        {
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " takes at most ten stations after end A",
                         "stressOutput");
            return card;
        }
        BeamPropStation station;
        if (const std::string_view stressOutput = reader.text(field); (stressOutput == "Yes"))
            station.stressOutput = BeamStressOutput::Yes;
        else if ((stressOutput == "YesSameAsEndA"))
            station.stressOutput = BeamStressOutput::YesA;
        else if ((stressOutput == "No"))
            station.stressOutput = BeamStressOutput::No;
        else
            reader.error(
                "card.bad-value",
                std::string("stressOutput") + " must be YES, YESA or NO, found \"" + std::string(stressOutput) + "\"",
                "stressOutput");
        station.stationRatio = reader.requiredReal(field + 1, "stationRatio");
        for (int index = 0; index < 6; ++index)
        {
            const auto value = reader.optionalReal(field + 2 + index, SECTION_NAMES[index]);
            station.*SECTION_MEMBERS[index] = value.value_or(NOT_GIVEN);
        }
        field += FIELDS_PER_LINE;
        if (station.stressOutput == BeamStressOutput::Yes)
        {
            readStressPoints(reader, field, station);
            field += FIELDS_PER_LINE;
        }
        card.stations.push_back(station);
    }

    // The last two lines. Blank values: the usual default, or the value at end A where the card says so.
    card.shearFactor1 = reader.real(field, "shearFactor1", 1.0);
    card.shearFactor2 = reader.real(field + 1, "shearFactor2", 1.0);
    card.shearRelief1 = reader.real(field + 2, "shearRelief1", 0.0);
    card.shearRelief2 = reader.real(field + 3, "shearRelief2", 0.0);
    card.nsmInertiaA = reader.real(field + 4, "nsmInertia", 0.0);
    card.nsmInertiaB = reader.real(field + 5, "nsmInertia", card.nsmInertiaA);
    card.warpingCoefficientA = reader.real(field + 6, "warpingCoefficient", 0.0);
    card.warpingCoefficientB = reader.real(field + 7, "warpingCoefficient", card.warpingCoefficientA);
    card.nsmCenterYA = reader.real(field + 8, "nsmCenterY", 0.0);
    card.nsmCenterZA = reader.real(field + 9, "nsmCenterZ", 0.0);
    card.nsmCenterYB = reader.real(field + 10, "nsmCenterY", card.nsmCenterYA);
    card.nsmCenterZB = reader.real(field + 11, "nsmCenterZ", card.nsmCenterZA);
    card.neutralAxisYA = reader.real(field + 12, "neutralAxisY", 0.0);
    card.neutralAxisZA = reader.real(field + 13, "neutralAxisZ", 0.0);
    card.neutralAxisYB = reader.real(field + 14, "neutralAxisY", card.neutralAxisYA);
    card.neutralAxisZB = reader.real(field + 15, "neutralAxisZ", card.neutralAxisZA);
    if (reader.lastUsedField() >= field + 2 * FIELDS_PER_LINE)
        reader.error("card.bad-value", "the card has fields after the last line of a " + std::string(reader.cardName()),
                     "neutralAxisZ");

    if (reader.failed())
        return card;
    if (!resolveBlankSections(card.stations))
    {
        reader.error("card.missing-field", "one station must be at " + std::string("stationRatio") + " = 1.0 (end B)",
                     "stationRatio");
        return card;
    }
    reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamPropFormat::check(const BeamPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.stations.empty() || card.stations.size() > BeamPropCard::MAX_STATIONS)
        return CardProblem{"card.bad-value", "the card has end A and at most ten more stations", "stationRatio"};

    const BeamPropStation& endA = card.stations.front();
    if (endA.stationRatio != 0.0)
        return CardProblem{"card.bad-value", "end A is at " + std::string(CardProblem::FIELD) + " = 0.0",
                           "stationRatio"};
    if (endA.stressOutput == BeamStressOutput::YesA)
        return CardProblem{"card.bad-value", "end A has no YESA output: its stress points are the reference",
                           "stressOutput"};
    if (endA.stressOutput == BeamStressOutput::No && card.stations.size() == 1)
        return CardProblem{"card.bad-value",
                           "the line of stress points at end A can only be left out when other stations follow",
                           "stressOutput"};

    for (std::size_t index = 0; index < card.stations.size(); ++index)
    {
        const BeamPropStation& station = card.stations[index];
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
        if (station.area < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "area"};
        if (station.bendingInertia1 < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "bendingInertia1"};
        if (station.bendingInertia2 < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "bendingInertia2"};
        if (station.torsionConstant < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "torsionConstant"};
        if (station.productInertia12 * station.productInertia12 > station.bendingInertia1 * station.bendingInertia2)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " squared must not exceed " +
                                   CardProblem::fieldNamed("bendingInertia1") + " times " +
                                   CardProblem::fieldNamed("bendingInertia2"),
                               "productInertia12"};
    }
    if (card.stations.size() > 1 && endBIndex(card.stations) == card.stations.size())
        return CardProblem{"card.missing-field",
                           "one station must be at " + std::string(CardProblem::FIELD) + " = 1.0 (end B)",
                           "stationRatio"};
    if (card.shearFactor1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor1"};
    if (card.shearFactor2 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor2"};
    return std::nullopt;
}

void BeamPropFormat::write(const BeamPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    const BeamPropStation& endA = card.stations.front();
    writer.real(3, endA.area);
    writer.real(4, endA.bendingInertia1);
    writer.real(5, endA.bendingInertia2);
    writer.realUnlessDefault(6, endA.productInertia12, 0.0);
    writer.realUnlessDefault(7, endA.torsionConstant, 0.0);
    writer.realUnlessDefault(8, endA.nonStructMass, 0.0);
    int field = FIRST_STATION_FIELD;
    if (endA.stressOutput == BeamStressOutput::Yes)
    {
        writeStressPoints(writer, field, endA);
        field += FIELDS_PER_LINE;
    }
    for (std::size_t index = 1; index < card.stations.size(); ++index)
    {
        const BeamPropStation& station = card.stations[index];
        writer.text(field, outputName(station.stressOutput));
        writer.real(field + 1, station.stationRatio);
        // The section properties of a station can be interpolated from the two ends: always written, so that the
        // station does not move with them.
        for (int property = 0; property < 6; ++property)
            writer.real(field + 2 + property, station.*SECTION_MEMBERS[property]);
        field += FIELDS_PER_LINE;
        if (station.stressOutput == BeamStressOutput::Yes)
        {
            writeStressPoints(writer, field, station);
            field += FIELDS_PER_LINE;
        }
    }

    // The last two lines are left out when everything in them is the plain default. The "B" values default to those
    // of end A, so they are written whenever their line is.
    const bool firstLineGiven = card.shearFactor1 != 1.0 || card.shearFactor2 != 1.0 || card.shearRelief1 != 0.0 ||
                                card.shearRelief2 != 0.0 || card.nsmInertiaA != 0.0 || card.nsmInertiaB != 0.0 ||
                                card.warpingCoefficientA != 0.0 || card.warpingCoefficientB != 0.0;
    const bool secondLineGiven = card.nsmCenterYA != 0.0 || card.nsmCenterZA != 0.0 || card.nsmCenterYB != 0.0 ||
                                 card.nsmCenterZB != 0.0 || card.neutralAxisYA != 0.0 || card.neutralAxisZA != 0.0 ||
                                 card.neutralAxisYB != 0.0 || card.neutralAxisZB != 0.0;
    if (firstLineGiven)
    {
        writer.realUnlessDefault(field, card.shearFactor1, 1.0);
        writer.realUnlessDefault(field + 1, card.shearFactor2, 1.0);
        writer.realUnlessDefault(field + 2, card.shearRelief1, 0.0);
        writer.realUnlessDefault(field + 3, card.shearRelief2, 0.0);
        writer.realUnlessDefault(field + 4, card.nsmInertiaA, 0.0);
        writer.real(field + 5, card.nsmInertiaB);
        writer.realUnlessDefault(field + 6, card.warpingCoefficientA, 0.0);
        writer.real(field + 7, card.warpingCoefficientB);
    }
    if (secondLineGiven)
    {
        writer.realUnlessDefault(field + 8, card.nsmCenterYA, 0.0);
        writer.realUnlessDefault(field + 9, card.nsmCenterZA, 0.0);
        writer.real(field + 10, card.nsmCenterYB);
        writer.real(field + 11, card.nsmCenterZB);
        writer.realUnlessDefault(field + 12, card.neutralAxisYA, 0.0);
        writer.realUnlessDefault(field + 13, card.neutralAxisZA, 0.0);
        writer.real(field + 14, card.neutralAxisYB);
        writer.real(field + 15, card.neutralAxisZB);
    }
}

namespace
{

// The four station lists of a card, as the table stores them.
struct StationLists
{
    std::vector<std::uint8_t> output;
    std::vector<double> position;
    std::vector<std::array<double, 6>> section;
    std::vector<std::array<double, 8>> stress;
};

StationLists listsOf(const std::vector<BeamPropStation>& stations)
{
    StationLists lists;
    for (const BeamPropStation& station : stations)
    {
        lists.output.push_back(static_cast<std::uint8_t>(station.stressOutput));
        lists.position.push_back(station.stationRatio);
        lists.section.push_back({station.area, station.bendingInertia1, station.bendingInertia2,
                                 station.productInertia12, station.torsionConstant, station.nonStructMass});
        lists.stress.push_back({station.stressPointC[0], station.stressPointC[1], station.stressPointD[0],
                                station.stressPointD[1], station.stressPointE[0], station.stressPointE[1],
                                station.stressPointF[0], station.stressPointF[1]});
    }
    return lists;
}

} // namespace

std::uint32_t BeamPropTable::append(const BeamPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    const StationLists lists = listsOf(card.stations);
    m_stationOutput.push(lists.output);
    m_stationRatio.push(lists.position);
    m_stationSection.push(lists.section);
    m_stationStress.push(lists.stress);
    m_shearFactor.push({card.shearFactor1, card.shearFactor2});
    m_shearRelief.push({card.shearRelief1, card.shearRelief2});
    m_nsmInertia.push({card.nsmInertiaA, card.nsmInertiaB});
    m_warpingCoefficient.push({card.warpingCoefficientA, card.warpingCoefficientB});
    m_nsmCenter.push({card.nsmCenterYA, card.nsmCenterZA, card.nsmCenterYB, card.nsmCenterZB});
    m_neutralAxis.push({card.neutralAxisYA, card.neutralAxisZA, card.neutralAxisYB, card.neutralAxisZB});
    return row;
}

BeamPropCard BeamPropTable::card(std::uint32_t row) const
{
    BeamPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    const auto output = m_stationOutput.get(row);
    const auto position = m_stationRatio.get(row);
    const auto section = m_stationSection.get(row);
    const auto stress = m_stationStress.get(row);
    card.stations.clear();
    for (std::size_t index = 0; index < position.size(); ++index)
    {
        BeamPropStation station;
        station.stressOutput = static_cast<BeamStressOutput>(output[index]);
        station.stationRatio = position[index];
        station.area = section[index][0];
        station.bendingInertia1 = section[index][1];
        station.bendingInertia2 = section[index][2];
        station.productInertia12 = section[index][3];
        station.torsionConstant = section[index][4];
        station.nonStructMass = section[index][5];
        station.stressPointC = {stress[index][0], stress[index][1]};
        station.stressPointD = {stress[index][2], stress[index][3]};
        station.stressPointE = {stress[index][4], stress[index][5]};
        station.stressPointF = {stress[index][6], stress[index][7]};
        card.stations.push_back(station);
    }
    const std::array<double, 2> shear = m_shearFactor.get(row);
    card.shearFactor1 = shear[0];
    card.shearFactor2 = shear[1];
    const std::array<double, 2> relief = m_shearRelief.get(row);
    card.shearRelief1 = relief[0];
    card.shearRelief2 = relief[1];
    const std::array<double, 2> inertia = m_nsmInertia.get(row);
    card.nsmInertiaA = inertia[0];
    card.nsmInertiaB = inertia[1];
    const std::array<double, 2> warping = m_warpingCoefficient.get(row);
    card.warpingCoefficientA = warping[0];
    card.warpingCoefficientB = warping[1];
    const std::array<double, 4> mass = m_nsmCenter.get(row);
    card.nsmCenterYA = mass[0];
    card.nsmCenterZA = mass[1];
    card.nsmCenterYB = mass[2];
    card.nsmCenterZB = mass[3];
    const std::array<double, 4> axis = m_neutralAxis.get(row);
    card.neutralAxisYA = axis[0];
    card.neutralAxisZA = axis[1];
    card.neutralAxisYB = axis[2];
    card.neutralAxisZB = axis[3];
    return card;
}

void BeamPropTable::store(std::uint32_t row, const BeamPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    const StationLists lists = listsOf(card.stations);
    m_stationOutput.set(row, lists.output);
    m_stationRatio.set(row, lists.position);
    m_stationSection.set(row, lists.section);
    m_stationStress.set(row, lists.stress);
    m_shearFactor.set(row, {card.shearFactor1, card.shearFactor2});
    m_shearRelief.set(row, {card.shearRelief1, card.shearRelief2});
    m_nsmInertia.set(row, {card.nsmInertiaA, card.nsmInertiaB});
    m_warpingCoefficient.set(row, {card.warpingCoefficientA, card.warpingCoefficientB});
    m_nsmCenter.set(row, {card.nsmCenterYA, card.nsmCenterZA, card.nsmCenterYB, card.nsmCenterZB});
    m_neutralAxis.set(row, {card.neutralAxisYA, card.neutralAxisZA, card.neutralAxisYB, card.neutralAxisZB});
}

void BeamPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_stationOutput.keepRows(rows);
    m_stationRatio.keepRows(rows);
    m_stationSection.keepRows(rows);
    m_stationStress.keepRows(rows);
    m_shearFactor.keepRows(rows);
    m_shearRelief.keepRows(rows);
    m_nsmInertia.keepRows(rows);
    m_warpingCoefficient.keepRows(rows);
    m_nsmCenter.keepRows(rows);
    m_neutralAxis.keepRows(rows);
}

void BeamPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
