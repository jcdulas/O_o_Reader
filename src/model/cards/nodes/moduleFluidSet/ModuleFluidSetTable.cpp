#include "oor/model/cards/nodes/moduleFluidSet/ModuleFluidSetTable.hpp"

#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ID_FIELD = 2;

} // namespace

std::optional<ModuleFluidSetCard> ModuleFluidSetFormat::read(FieldReader& reader)
{
    ModuleFluidSetCard card;
    card.moduleId = reader.integer(1, "moduleId", 0);
    card.ids = readIdRuns(reader, FIRST_ID_FIELD, "ids");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleFluidSetFormat::check(const ModuleFluidSetCard& card)
{
    if (const auto problem = checkBoundaryModule(card.moduleId, "moduleId"))
        return problem;
    return checkIdRuns(card.ids, "ids");
}

void ModuleFluidSetFormat::write(const ModuleFluidSetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.moduleId);
    writeIdRuns(writer, FIRST_ID_FIELD, card.ids, writer.compressesRanges());
}

std::uint32_t ModuleFluidSetTable::append(const ModuleFluidSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_moduleId.push(card.moduleId);
    m_cards.push(card);
    return row;
}

void ModuleFluidSetTable::store(std::uint32_t row, const ModuleFluidSetCard& card)
{
    m_moduleId.set(row, card.moduleId);
    m_cards.set(row, card);
}

void ModuleFluidSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_moduleId.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
