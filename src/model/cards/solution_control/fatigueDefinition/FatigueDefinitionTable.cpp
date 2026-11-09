#include "oor/model/cards/solution_control/fatigueDefinition/FatigueDefinitionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"

#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr std::array<std::string_view, 4> SECTION_KEYWORDS{"ElemSet", "SpotWeld", "SeamWeld", "ExcludedElemSet"};

constexpr int PAIRS_PER_LINE = 3; // ELSIDi and PFTGIDi side by side, from field 3
constexpr int SETS_PER_LINE = 7;  // XELSIDi in fields 3 to 9
constexpr int FIRST_DATA_FORMAT_FIELD = 3;

// The format reference field of the ELSIDi of the pair `position` (0 to 2) of a line; its PFTGIDi is the next field.
constexpr int pairFormatField(int position) noexcept
{
    return FIRST_DATA_FORMAT_FIELD + 2 * position;
}

// An ELSET or SPOTW section: pairs of a set of elements and its properties, three to a line.
void readPairs(FieldReader& reader, const LineSection& section, std::vector<FatigueDefinitionPair>& pairs)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        requireBlankLineFields(reader, line, 9, 9, section.keyword);
        for (int position = 0; position < PAIRS_PER_LINE; ++position)
        {
            const int elemSetField = fatigueField(line, pairFormatField(position));
            const int propField = elemSetField + 1;
            if (reader.isBlank(elemSetField) && reader.isBlank(propField))
                continue;
            if (reader.isBlank(elemSetField))
            {
                reader.error("card.missing-field",
                             std::string("fatiguePropIds") + " needs the set of elements " + std::string("elemSetIds") +
                                 " before it",
                             "elemSetIds");
                return;
            }
            FatigueDefinitionPair pair;
            pair.elemSetId = reader.requiredInteger(elemSetField, "elemSetIds");
            pair.fatiguePropId = reader.integer(propField, "fatiguePropIds", 0);
            pairs.push_back(pair);
        }
    }
    if (pairs.empty() && !reader.failed())
        reader.error("card.missing-field", "an " + section.keyword + " line needs at least one set of elements, ELSID1",
                     "elemSetIds");
}

// A SEAMW section: one group on each line, ELSIDi PFTGIDi NDSIDi WELDi TYPEi in fields 3 to 7.
void readSeams(FieldReader& reader, const LineSection& section, std::vector<FatigueDefinitionSeam>& seams)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        requireBlankLineFields(reader, line, 8, 9, "SeamWeld");
        if (lineHasData(reader, line) == false)
            continue;
        FatigueDefinitionSeam seam;
        seam.elemSetId = reader.requiredInteger(fatigueField(line, 3), "elemSetIds");
        seam.fatiguePropId = reader.integer(fatigueField(line, 4), "fatiguePropIds", 0);
        seam.nodeSetId = reader.integer(fatigueField(line, 5), "nodeSetIds", 0);
        seam.weld = readKeyword(reader, fatigueField(line, 6), "weldType", FATIGUE_DEFINITION_WELD_WORDS,
                                FatigueDefinitionWeld::Generic);
        seam.location = readKeyword(reader, fatigueField(line, 7), "weldLocation", FATIGUE_DEFINITION_LOCATION_WORDS,
                                    defaultWeldLocation(seam.weld));
        seams.push_back(seam);
    }
    if (seams.empty() && !reader.failed())
        reader.error("card.missing-field", "a SEAMW line needs at least one group, ELSID1", "elemSetIds");
}

// An XELSET section: the sets of elements that are left out, seven to a line.
void readExcluded(FieldReader& reader, const LineSection& section, std::vector<base::EntityId>& sets)
{
    for (int line = section.firstLine; line <= section.lastLine && !reader.failed(); ++line)
    {
        for (int position = 0; position < SETS_PER_LINE; ++position)
        {
            const int field = fatigueField(line, FIRST_DATA_FORMAT_FIELD + position);
            if (!reader.isBlank(field))
                sets.push_back(reader.requiredInteger(field, "excludedElemSetIds"));
        }
    }
    if (sets.empty() && !reader.failed())
        reader.error("card.missing-field", "an XELSET line needs at least one set of elements, XELSID1",
                     "excludedElemSetIds");
}

void readSections(FieldReader& reader, FatigueDefinitionCard& card)
{
    bool seen[SECTION_KEYWORDS.size()] = {};
    for (const LineSection& section : cutLineSections(reader, SECTION_KEYWORDS))
    {
        if (reader.failed())
            return;
        std::size_t which = 0;
        while (SECTION_KEYWORDS[which] != section.keyword)
            ++which;
        if (seen[which])
        {
            reader.error("card.bad-value",
                         "an " + std::string(reader.cardName()) + " has one " + section.keyword + " line",
                         std::string(section.keyword).c_str());
            return;
        }
        seen[which] = true;
        switch (which)
        {
        case 0:
            readPairs(reader, section, card.elemSets);
            break;
        case 1:
            readPairs(reader, section, card.spotWelds);
            break;
        case 2:
            readSeams(reader, section, card.seamWelds);
            break;
        default:
            readExcluded(reader, section, card.excludedElemSetIds);
            break;
        }
    }
}

// Writes the pairs of an ELSET or SPOTW section from line `line`; returns the line after it.
int writePairs(FieldWriter& writer, int line, std::string_view keyword, const std::vector<FatigueDefinitionPair>& pairs)
{
    if (pairs.empty())
        return line;
    writer.text(fatigueField(line, 2), keyword);
    for (std::size_t index = 0; index < pairs.size(); ++index)
    {
        const int pairLine = line + static_cast<int>(index) / PAIRS_PER_LINE;
        const int elemSetField = fatigueField(pairLine, pairFormatField(static_cast<int>(index) % PAIRS_PER_LINE));
        writer.integer(elemSetField, pairs[index].elemSetId);
        if (pairs[index].fatiguePropId > 0)
            writer.integer(elemSetField + 1, pairs[index].fatiguePropId);
    }
    return line + (static_cast<int>(pairs.size()) + PAIRS_PER_LINE - 1) / PAIRS_PER_LINE;
}

} // namespace

std::optional<FatigueDefinitionCard> FatigueDefinitionFormat::read(FieldReader& reader)
{
    FatigueDefinitionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.topStressCount = reader.optionalReal(2, "topStressCount");
    card.fatiguePropIds = reader.integer(3, "fatiguePropIds", 0);
    card.topDamageCount = reader.optionalReal(4, "topDamageCount");
    card.maxEntities = reader.optionalInteger(5, "maxEntities");
    card.maxOutputEntities = reader.integer(6, "maxOutputEntities", 100);
    card.hotSpotCount = reader.optionalInteger(7, "hotSpotCount");
    card.hotSpotGate = reader.integer(8, "hotSpotGate", 0);
    readSections(reader, card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueDefinitionFormat::check(const FatigueDefinitionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.topStressCount)
    {
        if (const auto problem = checkRealRange(*card.topStressCount, "topStressCount", -100.0, 100.0))
            return problem;
    }
    if (const auto problem = checkOptionalId(card.fatiguePropIds, "fatiguePropIds"))
        return problem;
    if (card.topDamageCount)
    {
        if (const auto problem = checkReal(*card.topDamageCount, "topDamageCount", RealRule::Positive))
            return problem;
        if (*card.topDamageCount > 100.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be above 100",
                               "topDamageCount"};
    }
    if (card.maxOutputEntities <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "maxOutputEntities"};
    if (card.hotSpotCount && *card.hotSpotCount <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "hotSpotCount"};
    if (card.hotSpotGate < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "hotSpotGate"};
    for (const std::vector<FatigueDefinitionPair>* pairs : {&card.elemSets, &card.spotWelds})
    {
        for (const FatigueDefinitionPair& pair : *pairs)
        {
            if (const auto problem = checkRequiredId(pair.elemSetId, "elemSetIds"))
                return problem;
            if (const auto problem = checkOptionalId(pair.fatiguePropId, "fatiguePropIds"))
                return problem;
        }
    }
    for (const FatigueDefinitionSeam& seam : card.seamWelds)
    {
        if (const auto problem = checkRequiredId(seam.elemSetId, "elemSetIds"))
            return problem;
        if (const auto problem = checkOptionalId(seam.fatiguePropId, "fatiguePropIds"))
            return problem;
        if (const auto problem = checkOptionalId(seam.nodeSetId, "nodeSetIds"))
            return problem;
    }
    for (const base::EntityId set : card.excludedElemSetIds)
    {
        if (const auto problem = checkRequiredId(set, "excludedElemSetIds"))
            return problem;
    }
    return std::nullopt;
}

void FatigueDefinitionFormat::write(const FatigueDefinitionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.topStressCount)
        writer.real(2, *card.topStressCount);
    if (card.fatiguePropIds > 0)
        writer.integer(3, card.fatiguePropIds);
    if (card.topDamageCount)
        writer.real(4, *card.topDamageCount);
    if (card.maxEntities)
        writer.integer(5, *card.maxEntities);
    writer.integerUnlessDefault(6, card.maxOutputEntities, 100);
    if (card.hotSpotCount)
        writer.integer(7, *card.hotSpotCount);
    writer.integerUnlessDefault(8, card.hotSpotGate, 0);
    int line = 1;
    line = writePairs(writer, line, "ElemSet", card.elemSets);
    line = writePairs(writer, line, "SpotWeld", card.spotWelds);
    for (std::size_t index = 0; index < card.seamWelds.size(); ++index)
    {
        const FatigueDefinitionSeam& seam = card.seamWelds[index];
        const int seamLine = line + static_cast<int>(index);
        if (index == 0)
            writer.text(fatigueField(seamLine, 2), "SeamWeld");
        writer.integer(fatigueField(seamLine, 3), seam.elemSetId);
        if (seam.fatiguePropId > 0)
            writer.integer(fatigueField(seamLine, 4), seam.fatiguePropId);
        if (seam.nodeSetId > 0)
            writer.integer(fatigueField(seamLine, 5), seam.nodeSetId);
        writeKeywordUnlessDefault(writer, fatigueField(seamLine, 6), FATIGUE_DEFINITION_WELD_WORDS, seam.weld,
                                  FatigueDefinitionWeld::Generic);
        writeKeyword(writer, fatigueField(seamLine, 7), FATIGUE_DEFINITION_LOCATION_WORDS,
                     seam.location); // its default depends on WELD
    }
    line += static_cast<int>(card.seamWelds.size());
    if (!card.excludedElemSetIds.empty())
    {
        writer.text(fatigueField(line, 2), "ExcludedElemSet");
        for (std::size_t index = 0; index < card.excludedElemSetIds.size(); ++index)
        {
            const int setLine = line + static_cast<int>(index) / SETS_PER_LINE;
            writer.integer(fatigueField(setLine, FIRST_DATA_FORMAT_FIELD + static_cast<int>(index) % SETS_PER_LINE),
                           card.excludedElemSetIds[index]);
        }
    }
}

std::uint32_t FatigueDefinitionTable::append(const FatigueDefinitionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueDefinitionTable::store(std::uint32_t row, const FatigueDefinitionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueDefinitionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueDefinitionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueDefinitionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FatigueDefinitionCard& card = m_cards.edit(row);
    visitId(visitor, "fatiguePropIds", ReferenceTarget::FatigueProperty, card.fatiguePropIds);
    for (std::vector<FatigueDefinitionPair>* pairs : {&card.elemSets, &card.spotWelds})
    {
        for (FatigueDefinitionPair& pair : *pairs)
        {
            visitId(visitor, "elemSetIds", ReferenceTarget::Set, pair.elemSetId);
            visitId(visitor, "fatiguePropIds", ReferenceTarget::FatigueProperty, pair.fatiguePropId);
        }
    }
    for (FatigueDefinitionSeam& seam : card.seamWelds)
    {
        visitId(visitor, "elemSetIds", ReferenceTarget::Set, seam.elemSetId);
        visitId(visitor, "fatiguePropIds", ReferenceTarget::FatigueProperty, seam.fatiguePropId);
        visitId(visitor, "nodeSetIds", ReferenceTarget::Set, seam.nodeSetId);
    }
    for (base::EntityId& set : card.excludedElemSetIds)
        visitId(visitor, "excludedElemSetIds", ReferenceTarget::Set, set);
}

} // namespace oor::model
