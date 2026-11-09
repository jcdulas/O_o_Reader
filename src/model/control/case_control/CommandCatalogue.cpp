#include "oor/model/control/case_control/CommandCatalogue.hpp"

#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

const CommandCatalogue& CommandCatalogue::instance()
{
    static const CommandCatalogue CATALOGUE;
    return CATALOGUE;
}

CommandCatalogue::CommandCatalogue()
{
    m_definitions.reserve(600); // the families keep references to the definition they just added: no reallocation
    defineSubcaseCommands(*this);
    defineLoadAndConstraintCommands(*this);
    defineSolutionCommands(*this);
    defineMatrixCommands(*this);
    defineAeroelasticityCommands(*this);
    defineOptimisationCommands(*this);
    defineOutputControlCommands(*this);
    defineOutputRequestCommands(*this);
    defineSuperelementCommands(*this);
    defineContactAndModuleCommands(*this);
    definePlotCommands(*this);
    defineXyPlotCommands(*this);
    buildIndexes();
}

CommandDefinition& CommandCatalogue::add(std::string name, CommandShape shape)
{
    CommandDefinition& definition = m_definitions.emplace_back();
    definition.name = std::move(name);
    definition.shape = shape;
    return definition;
}

CommandDefinition& CommandCatalogue::integer(std::string name)
{
    return add(std::move(name), CommandShape::Integer);
}

CommandDefinition& CommandCatalogue::integerOrKeyword(std::string name)
{
    return add(std::move(name), CommandShape::IntegerOrKeyword);
}

CommandDefinition& CommandCatalogue::real(std::string name)
{
    return add(std::move(name), CommandShape::Real);
}

CommandDefinition& CommandCatalogue::keyword(std::string name)
{
    return add(std::move(name), CommandShape::Keyword);
}

CommandDefinition& CommandCatalogue::text(std::string name)
{
    return add(std::move(name), CommandShape::Text);
}

CommandDefinition& CommandCatalogue::integerList(std::string name)
{
    return add(std::move(name), CommandShape::IntegerList);
}

CommandDefinition& CommandCatalogue::realList(std::string name)
{
    return add(std::move(name), CommandShape::RealList);
}

CommandDefinition& CommandCatalogue::output(std::string name)
{
    return add(std::move(name), CommandShape::Output);
}

CommandDefinition& CommandCatalogue::flag(std::string name)
{
    return add(std::move(name), CommandShape::Flag);
}

CommandDefinition& CommandCatalogue::subcaseHeader(std::string name, SubcaseKind kind)
{
    return add(std::move(name), CommandShape::SubcaseHeader).ofSubcaseKind(kind);
}

CommandDefinition& CommandCatalogue::setDefinition(std::string name)
{
    return add(std::move(name), CommandShape::SetDefinition);
}

CommandDefinition& CommandCatalogue::parameter(std::string name)
{
    return add(std::move(name), CommandShape::Parameter);
}

CommandDefinition& CommandCatalogue::blockHeader(std::string name, std::string blockKind)
{
    return add(std::move(name), CommandShape::BlockHeader).ofBlockKind(std::move(blockKind));
}

void CommandCatalogue::buildIndexes()
{
    for (std::size_t index = 0; index < m_definitions.size(); ++index)
    {
        if (!m_byName.emplace(m_definitions[index].name, index).second)
            m_duplicateNames.push_back(m_definitions[index].name);
    }
}

const CommandDefinition* CommandCatalogue::findByName(std::string_view name) const
{
    const auto found = m_byName.find(std::string(name));
    return found == m_byName.end() ? nullptr : &m_definitions[found->second];
}

} // namespace oor::model
