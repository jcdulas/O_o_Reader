#include "oor/model/control/case_control/Subcase.hpp"

namespace oor::model
{

std::string_view subcaseKindName(SubcaseKind kind) noexcept
{
    switch (kind)
    {
    case SubcaseKind::Global:
        return "global";
    case SubcaseKind::Subcase:
        return "subcase";
    case SubcaseKind::SubcaseCombination:
        return "subcaseCombination";
    case SubcaseKind::SymmetrySubcase:
        return "symmetrySubcase";
    case SubcaseKind::SymmetryCombination:
        return "symmetryCombination";
    case SubcaseKind::RepeatSubcase:
        return "repeatSubcase";
    case SubcaseKind::Step:
        return "step";
    case SubcaseKind::Substep:
        return "substep";
    case SubcaseKind::AuxiliaryCase:
        return "auxiliaryCase";
    }
    return "subcase";
}

std::optional<SubcaseKind> subcaseKindOf(std::string_view name) noexcept
{
    for (const SubcaseKind kind :
         {SubcaseKind::Global, SubcaseKind::Subcase, SubcaseKind::SubcaseCombination, SubcaseKind::SymmetrySubcase,
          SubcaseKind::SymmetryCombination, SubcaseKind::RepeatSubcase, SubcaseKind::Step, SubcaseKind::Substep,
          SubcaseKind::AuxiliaryCase})
    {
        if (subcaseKindName(kind) == name)
            return kind;
    }
    return std::nullopt;
}

std::string_view subcaseCommandName(SubcaseKind kind) noexcept
{
    switch (kind)
    {
    case SubcaseKind::Global:
        return {};
    case SubcaseKind::Subcase:
        return "SubcaseStart";
    case SubcaseKind::SubcaseCombination:
        return "SubcaseCombination";
    case SubcaseKind::SymmetrySubcase:
        return "SymmetrySubcase";
    case SubcaseKind::SymmetryCombination:
        return "SymmetryCombination";
    case SubcaseKind::RepeatSubcase:
        return "RepeatSubcase";
    case SubcaseKind::Step:
        return "StepStart";
    case SubcaseKind::Substep:
        return "SubstepStart";
    case SubcaseKind::AuxiliaryCase:
        return "AuxiliaryCase";
    }
    return "SubcaseStart";
}

std::string subcaseLabel(SubcaseKind kind)
{
    if (kind == SubcaseKind::Global)
        return "GLOBAL";
    return std::string(subcaseCommandName(kind));
}

} // namespace oor::model
