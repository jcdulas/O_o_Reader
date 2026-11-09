#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaDesignFields.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4DesignFields.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineDesignFields.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3DesignFields.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatDesignFields.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DDesignFields.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/properties/barProp/BarPropDesignFields.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropDesignFields.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropDesignFields.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropDesignFields.hpp"
#include "oor/model/cards/properties/compProp/CompPropDesignFields.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalDesignFields.hpp"
#include "oor/model/cards/properties/rodProp/RodPropDesignFields.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropDesignFields.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropDesignFields.hpp"
#include "oor/model/cards/properties/springProp/SpringPropDesignFields.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropDesignFields.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/design/DesignableCard.hpp"

#include <array>

namespace oor::model
{

namespace
{

struct Entry
{
    std::string_view name;
    const DesignableCard* card;
};

// The card types a design relation can drive. This is the one place that lists them: a card type that is not here is
// refused by `apply` and by the check `design.relation-target`. Adding a card type is one function file next to the
// card and one line here.
const std::array<Entry, 17>& entries()
{
    static const std::array<Entry, 17> ENTRIES = {{
        // properties (DVPREL1 and DVPREL2)
        {"ShellProp", &shellPropDesignFields()},
        {"BarProp", &barPropDesignFields()},
        {"BarShapeProp", &barShapePropDesignFields()},
        {"BeamProp", &beamPropDesignFields()},
        {"BeamShapeProp", &beamShapePropDesignFields()},
        {"CompProp", &compPropDesignFields()},
        {"CompPropGlobal", &compPropGlobalDesignFields()},
        {"RodProp", &rodPropDesignFields()},
        {"SpringProp", &springPropDesignFields()},
        {"ShearPanelProp", &shearPanelPropDesignFields()},
        {"TubeProp", &tubePropDesignFields()},
        // materials (DVMREL1 and DVMREL2)
        {"IsoMat", &isoMatDesignFields()},
        {"OrthoMat2D", &orthoMat2DDesignFields()},
        // elements (DVCREL1 and DVCREL2)
        {"PointMassInertia", &pointMassInertiaDesignFields()},
        {"SpringDofInline", &springDofInlineDesignFields()},
        {"ShellQuad4", &shellQuad4DesignFields()},
        {"ShellTria3", &shellTria3DesignFields()},
    }};
    return ENTRIES;
}

} // namespace

const DesignableCard* findDesignableCard(std::string_view cardName)
{
    for (const Entry& entry : entries())
    {
        if (entry.name == cardName)
            return entry.card;
    }
    // The name may be the keyword a relation gives (TYPE = CQUAD4).
    const std::optional<std::uint16_t> type = typeOfDesignedCard(cardName);
    if (!type)
        return nullptr;
    const std::string_view typeName = CardRegistry::instance().entry(*type).info.name;
    for (const Entry& entry : entries())
    {
        if (entry.name == typeName)
            return entry.card;
    }
    return nullptr;
}

std::vector<std::string> designableCardNames()
{
    std::vector<std::string> names;
    for (const Entry& entry : entries())
        names.emplace_back(entry.name);
    return names;
}

} // namespace oor::model
