#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSectionFields.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One NLSTEP card: the control parameters of the load or time stepping of SOL 400 (mechanical, thermal and coupled
// analysis) and of the contact analysis of SOL 101. The Case Control command NLSTEP=ID selects it.
//
// Card layout: NLSTEP  ID  TOTTIME  CTRLDEF
//   then keyword lines, in any order, with the named fields of the format reference on them:
//     GENERAL  MAXITER  MINITER  MAXBIS  CREEP
//     LCNT     NINCC  CONVC  EPSUC  EPSPC  EPSWC  MAXDIVC  MAXBISC / MAXITERC  MINITERC          (SOL 101 only)
//     FIXED    NINC  NO
//     ADAPT    DTINITF  DTMINF  DTMAXF  NDESIR  SFACT  INTOUT  NSMAX / IDAMP  DAMP  CRITTID  IPHYS  LIMTAR  RSMALL RBIG
//     /
//              ADJUST  MSTEP  RB  UTOL
//     ARCLN    TYPE  DTINITFA  MINALR  MAXALR  (blank)  NDESIRA  NSMAXA
//     HEAT     CONVH  EPSUH  EPSPH  EPSWH  KMETHODH  KSTEPH / MAXQNH  MAXLSH  LSTOLH
//     MECH     CONV  EPSU  EPSP  EPSW  KMETHOD  KSTEP  MRCONV / MAXQN  MAXLS  LSTOL  FSTRESS
//     COUP     HGENPLAS  HGENFRIC
// (the values of a keyword are in the fields after it; the second and third lines of ADAPT, HEAT and MECH have no
// keyword). Only one of LCNT, FIXED, ADAPT and ARCLN may be given. With none of them the default is FIXED with 50
// increments. The CRITTID of the ADAPT line is the number of a TABSCTL, the user criteria of the load stepping (a
// `LoadStepControl` reference: `NonlinearStepControlTable::visitReferences`,
// `NonlinearStepControlFormat::criteriaTable`).
struct NonlinearStepControlCard
{
    base::EntityId id = 0;                     // referenced by the NLSTEP command
    double totalTime = 1.0;                    // the total time of the load case
    std::string controlSetting;                // QLINEAR, MILDLY, SEVERELY (SOL 400), LCPERF or LCACCU (SOL 101); blank
    std::vector<ContactNamedSection> sections; // the keyword lines with their fields
};

} // namespace oor::model
