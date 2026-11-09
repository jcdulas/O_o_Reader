#include "FastenerInputs.hpp"

#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

namespace oor::analysis::detail
{

namespace
{

// The SWLDPRM projection tolerance PROJTOL that applies to one element type: the global value, then the one of its
// section (a parameter after a CFAST or CWELD keyword applies to that type only).
void apply(const model::ConnectorSearchParamsParameter& parameter, double& projtol)
{
    if (parameter.name == "ProjectionTolerance")
        projtol = parameter.value.asReal();
}

double projtolFor(const model::Model& model, model::ConnectorSearchParamsElement element)
{
    double projtol = FastenerInput::DEFAULT_PROJTOL;
    const model::ConnectorSearchParamsTable* table = model.findTable<model::ConnectorSearchParamsTable>();
    if (table == nullptr)
        return projtol;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const model::ConnectorSearchParamsCard card = table->card(row);
        for (const model::ConnectorSearchParamsParameter& parameter : card.global)
            apply(parameter, projtol);
        for (const model::ConnectorSearchParamsSection& section : card.sections)
        {
            if (section.element != element)
                continue;
            for (const model::ConnectorSearchParamsParameter& parameter : section.parameters)
                apply(parameter, projtol);
        }
    }
    return projtol;
}

std::optional<base::Vec3> locationOf(const std::array<double, 3>& location)
{
    if (std::isnan(location[0]) || std::isnan(location[1]) || std::isnan(location[2]))
        return std::nullopt;
    return base::Vec3{location[0], location[1], location[2]};
}

// The shape letter of a GRIDID side: SPTYP gives one letter per side (QQ, QT, TT, TQ), one letter alone for a point to
// patch connection (Q, T); a side whose grid points are all blank has no face.
// The shapes of the two patches a SPTYP word names, one letter a side ('Q' four corners, 'T' three): QuadTriaPatch is
// "QT", QuadPatch (a point to a quadrilateral patch) is "Q"; empty for a blank or unknown word.
std::string patchShapeLetters(const std::string& patchShapes)
{
    static const std::array<std::pair<std::string_view, std::string_view>, 6> LETTERS = {{
        {"QuadQuadPatch", "QQ"},
        {"QuadTriaPatch", "QT"},
        {"TriaTriaPatch", "TT"},
        {"TriaQuadPatch", "TQ"},
        {"QuadPatch", "Q"},
        {"TriaPatch", "T"},
    }};
    for (const auto& [word, letters] : LETTERS)
    {
        if (patchShapes == word)
            return std::string(letters);
    }
    return {};
}

char shapeOfSide(const std::string& patchShapes, std::size_t side, const std::array<base::EntityId, 8>& grids)
{
    if (std::all_of(grids.begin(), grids.end(), [](base::EntityId grid) { return grid == 0; }))
        return 0;
    if (side < patchShapes.size() && (patchShapes[side] == 'Q' || patchShapes[side] == 'T'))
        return patchShapes[side];
    if (patchShapes.size() == 1 && side == 1)
        return 0;
    return grids[3] != 0 ? 'Q' : 'T'; // SPTYP blank: four corners or three
}

} // namespace

std::vector<FastenerInput> FastenerInputs::read(const model::Model& model)
{
    std::vector<FastenerInput> inputs;
    if (const model::FastenerTable* table = model.findTable<model::FastenerTable>())
    {
        const double projtol = projtolFor(model, model::ConnectorSearchParamsElement::Fastener);
        const std::uint16_t type = model::typeIndexOf<model::FastenerTable>();
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::FastenerCard card = table->card(row);
            FastenerInput input;
            input.card = model::CardLocation{type, row};
            input.eid = card.id;
            input.form = card.kind == "Elements" ? FastenerForm::Elem : FastenerForm::Prop;
            input.gs = card.spotNode;
            input.ga = card.nodeA;
            input.gb = card.nodeB;
            input.location = locationOf(card.location);
            input.ida = card.patchIdA;
            input.idb = card.patchIdB;
            input.projtol = projtol;
            inputs.push_back(std::move(input));
        }
    }
    if (const model::WeldTable* table = model.findTable<model::WeldTable>())
    {
        const double projtol = projtolFor(model, model::ConnectorSearchParamsElement::Weld);
        const std::uint16_t type = model::typeIndexOf<model::WeldTable>();
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::WeldCard card = table->card(row);
            FastenerInput input;
            input.card = model::CardLocation{type, row};
            input.eid = card.id;
            const std::string& word = card.kind;
            if (word == "ElemPatch")
                input.form = FastenerForm::Elpat;
            else if (word == "ElemIdPatch")
                input.form = FastenerForm::Elemid;
            else if (word == "NodeIdPatch")
                input.form = FastenerForm::Gridid;
            else if (word == "AlignedNodes")
                input.form = FastenerForm::Align;
            else
                input.form = FastenerForm::Partpat;
            input.gs = card.spotNode;
            input.ga = card.nodeA;
            input.gb = card.nodeB;
            input.location = locationOf(card.location);
            input.ida = card.patchIdA;
            input.idb = card.patchIdB;
            input.gai = card.patchNodesA;
            input.gbi = card.patchNodesB;
            const std::string patchShapes = patchShapeLetters(card.patchShapes);
            if (input.form == FastenerForm::Gridid)
            {
                input.shapeA = shapeOfSide(patchShapes, 0, card.patchNodesA);
                input.shapeB = shapeOfSide(patchShapes, 1, card.patchNodesB);
            }
            input.projtol = projtol;
            inputs.push_back(std::move(input));
        }
    }
    std::sort(inputs.begin(), inputs.end(),
              [](const FastenerInput& left, const FastenerInput& right) { return left.eid < right.eid; });
    return inputs;
}

} // namespace oor::analysis::detail
