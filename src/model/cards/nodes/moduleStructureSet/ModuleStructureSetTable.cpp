#include "oor/model/cards/nodes/moduleStructureSet/ModuleStructureSetTable.hpp"

#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 2;

} // namespace

std::optional<ModuleStructureSetCard> ModuleStructureSetFormat::read(FieldReader& reader)
{
    ModuleStructureSetCard card;
    card.moduleId = reader.integer(1, "moduleId", 0);
    card.ids = readIdRuns(reader, FIRST_ID_FIELD, "ids");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleStructureSetFormat::check(const ModuleStructureSetCard& card)
{
    if (const auto problem = checkBoundaryModule(card.moduleId, "moduleId"))
        return problem;
    return checkIdRuns(card.ids, "ids");
}

void ModuleStructureSetFormat::write(const ModuleStructureSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.moduleId);
    writeIdRuns(writer, FIRST_ID_FIELD, card.ids, writer.compressesRanges());
}

std::uint32_t ModuleStructureSetTable::append(const ModuleStructureSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_moduleId.push(card.moduleId);
    m_cards.push(card);
    return row;
}

void ModuleStructureSetTable::store(std::uint32_t row, const ModuleStructureSetCard& card)
{
    m_moduleId.set(row, card.moduleId);
    m_cards.set(row, card);
}

void ModuleStructureSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_moduleId.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
