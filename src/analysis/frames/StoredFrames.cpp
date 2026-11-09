#include "oor/analysis/frames/StoredFrames.hpp"

#include "FrameSupport.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/parameters/ParameterValues.hpp"

#include <algorithm>
#include <set>

namespace oor::analysis
{

using results::StoredFrame;

namespace
{

StoredFrameInfo unknown(std::string reason)
{
    StoredFrameInfo info;
    info.kind = StoredFrame::Unknown;
    info.reason = std::move(reason);
    return info;
}

StoredFrameInfo known(StoredFrame kind, std::string source, std::int32_t system = 0)
{
    StoredFrameInfo info;
    info.kind = kind;
    info.system = system;
    info.source = std::move(source);
    return info;
}

// The parameter that puts shell results in the material system, as a deck writes it ("PARAM,OMID"), for the messages.
std::string materialFrameText(const model::Model&)
{
    return std::string("Parameter") + "," + std::string("OutputMaterialFrame");
}

// The material system field of a solid property, as a deck writes it ("PSOLID CORDM"), for the messages.
std::string cordmText(const model::Model&)
{
    return std::string("SolidProp") + " " + std::string("matFrameId");
}

// The displacement system field of a grid point ("CD"), for the messages.
std::string cdText(const model::Model&)
{
    return std::string("displacementFrameId");
}

bool materialFrameApplies(std::string_view elementName) noexcept
{
    // PARAM,OMID is documented for the triangles and quadrilaterals CTRIA3, CQUAD4, CQUAD8 and the "TRIAR6" (read here
    // as CTRIAR and CTRIA6); the format reference lists nothing for CQUADR
    return elementName == "ShellQuad4" || elementName == "ShellQuad8" || elementName == "ShellTria3" ||
           elementName == "ShellTria6" || elementName == "ShellTria3Drilling";
}

StoredFrameInfo describeSolids(const model::Model& model, const results::ResultAttachment& attachment,
                               const results::ResultTable& table, const results::DecodeContext& context)
{
    std::set<base::EntityId> cordms;
    std::set<std::int32_t> checked;
    bool missingProperty = false;
    for (const std::int32_t id : detail::elementIdsOf(table, context))
    {
        if (!checked.insert(id).second)
            continue;
        const auto location = attachment.elementLocation(model, id);
        if (!location)
            continue;
        const auto pid = detail::elementPropertyId(model, *location);
        const auto cordm = pid ? detail::solidCordm(model, *pid) : std::nullopt;
        if (!cordm)
        {
            missingProperty = true;
            continue;
        }
        cordms.insert(*cordm);
    }
    const std::string cordmField = cordmText(model);
    if (missingProperty)
        return unknown("a solid element has a property that is not a " + std::string("SolidProp") + " (" +
                       std::string("CompSolidProp") + ", " + std::string("SolidPropHyperelastic") +
                       ", ...): its result frame is not defined here");
    if (cordms.empty())
        return known(StoredFrame::Basic, cordmField + " (no element of the table is linked to the model)");
    const SystemDefinitions definitions = readSystemDefinitions(model);
    std::set<base::EntityId> rectangular;
    for (const base::EntityId cordm : cordms)
    {
        if (cordm < 0)
            return unknown(cordmField + " " + std::to_string(cordm) +
                           ": the element system of a solid is not validated as a place where results are stored");
        if (cordm > 0)
        {
            const auto found = definitions.find(cordm);
            if (found == definitions.end())
                return unknown(cordmField + " " + std::to_string(cordm) + " is a system the model does not define");
            if (found->second.kind != CoordinateKind::Rectangular)
                return unknown(cordmField + " " + std::to_string(cordm) +
                               " is a cylindrical or spherical system: where in the element the system is evaluated "
                               "(the centre or each corner) is not documented, so the frame is not known");
            rectangular.insert(cordm);
        }
    }
    if (cordms.size() == 1)
        return *cordms.begin() == 0 ? known(StoredFrame::Basic, cordmField + " 0")
                                    : known(StoredFrame::Coordinate, cordmField, *cordms.begin());
    return known(StoredFrame::Mixed, cordmField + " (several values)");
}

} // namespace

MaterialFrameOutput StoredFrames::materialFrameOutput(const model::Model& model)
{
    // PARAM,OMID: the last card, else the format reference default (No). A value that is not one of the words Yes, No
    // and Both is Invalid.
    const std::optional<model::ParameterValue> value = model::parameterValue(model, "OutputMaterialFrame");
    if (!value)
        return MaterialFrameOutput::No;
    const std::string text = value->card.kind == model::ParameterKind::Text ? value->card.textValue : std::string();
    if (text == "Yes")
        return MaterialFrameOutput::Yes;
    if (text == "No")
        return MaterialFrameOutput::No;
    if (text == "Both")
        return MaterialFrameOutput::Both;
    return MaterialFrameOutput::Invalid;
}

std::optional<base::EntityId> StoredFrames::propertyOf(const model::Model& model, model::CardLocation location)
{
    return detail::elementPropertyId(model, location);
}

StoredFrameInfo StoredFrames::describeWithoutModel(const results::ResultTable& table)
{
    const results::ResultTableMeta& meta = table.meta();
    if (meta.derivation.rfind("inFrame(", 0) == 0)
    {
        StoredFrameInfo info = known(meta.storedFrame, "the operation that made the table", meta.frameSystem);
        info.reason = meta.frameReason;
        return info;
    }
    if (meta.origin == "memory" && meta.storedFrame != StoredFrame::Unknown)
        return known(meta.storedFrame, "declared by the program that made the results", meta.frameSystem);
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    if (family == FrameFamily::NodalVector || family == FrameFamily::NodeForceBalance)
        return known(meta.storedFrame, "the name of the table");
    return unknown(
        "no model is attached: the frame of an element result (the element, material, ply or a solid's "
        "material system) is the model's to say");
}

StoredFrameInfo StoredFrames::describe(const model::Model& model, const results::ResultAttachment& attachment,
                                       const results::ResultTable& table, const results::DecodeContext& context)
{
    const results::ResultTableMeta& meta = table.meta();
    if (meta.derivation.rfind("inFrame(", 0) == 0)
        return describeWithoutModel(table);
    // a table made in memory says its frame (a program that makes results knows it); only a frame it leaves unsaid is
    // the model's to find
    if (meta.origin == "memory" && meta.storedFrame != StoredFrame::Unknown)
        return describeWithoutModel(table);
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    switch (family)
    {
    case FrameFamily::NodalVector:
    case FrameFamily::NodeForceBalance:
        return known(meta.storedFrame,
                     "the name of the table (" + cdText(model) + " of each grid point from the model)");
    case FrameFamily::ShellStress:
    case FrameFamily::ShellStrain:
    case FrameFamily::ShellForce:
    {
        const std::string materialFrameParameter = materialFrameText(model);
        switch (materialFrameOutput(model))
        {
        case MaterialFrameOutput::No:
            return known(StoredFrame::Element, materialFrameParameter + " is NO or absent");
        case MaterialFrameOutput::Yes:
            if (materialFrameApplies(meta.elementName))
                return known(StoredFrame::Material, materialFrameParameter + ",YES");
            return unknown(materialFrameParameter + ",YES is defined for " + std::string("ShellTria3") + ", " +
                           std::string("ShellQuad4") + ", " + std::string("ShellQuad8") +
                           " and the six-node triangles only; nothing is known of " + meta.elementName);
        case MaterialFrameOutput::Both:
            return unknown(materialFrameParameter +
                           ",BOTH writes the results in both forms to the file and nothing in the tables says "
                           "which is the material form");
        case MaterialFrameOutput::Invalid:
            return unknown(materialFrameParameter + " has a value other than YES, NO or BOTH");
        }
        break;
    }
    case FrameFamily::PlyStress:
    case FrameFamily::PlyStrain:
        return known(StoredFrame::Ply, "the ply tables of laminated shells");
    case FrameFamily::SolidStress:
    case FrameFamily::SolidStrain:
        return describeSolids(model, attachment, table, context);
    default:
        break;
    }
    return unknown(FrameConventionTable::find(family, StoredFrame::Unknown, FrameTargetKind::Any).reason);
}

std::uint64_t StoredFrames::signature(const model::Model& model, base::ThreadPool* pool)
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    const auto add = [&](std::uint64_t value) { hash = RevisionStamp::hashBytes(hash, &value, sizeof(value)); };
    add(static_cast<std::uint64_t>(materialFrameOutput(model)));
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr)
        return hash;
    std::set<base::EntityId> systems;
    std::uint64_t cdSum = 0;
    for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
    {
        if (!grids->rows().isAlive(row))
            continue;
        const base::EntityId cd = grids->displacementFrameId(row);
        const std::uint64_t pair =
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(grids->idAtRow(row))) << 32U) |
            static_cast<std::uint32_t>(cd);
        cdSum += RevisionStamp::mix(pair);
        if (cd > 0)
            systems.insert(cd);
    }
    add(cdSum);
    try
    {
        BasicPositionCache& cache = BasicPositionCache::of(model, pool);
        const std::lock_guard<std::mutex> lock(cache.mutex());
        for (const base::EntityId cd : systems)
        {
            if (!cache.transform().hasSystem(cd))
            {
                add(0xdeadULL + static_cast<std::uint64_t>(cd));
                continue;
            }
            const CoordinateFrame& frame = cache.transform().frame(cd);
            add(static_cast<std::uint64_t>(cd));
            add(static_cast<std::uint64_t>(frame.kind));
            for (const double value : {frame.origin.x, frame.origin.y, frame.origin.z})
                hash = RevisionStamp::hashReal(hash, value);
            for (const double value : frame.axes.values)
                hash = RevisionStamp::hashReal(hash, value);
        }
    }
    catch (const CoordinateSystemError&)
    {
        add(0xbadULL); // a model whose systems cannot be resolved has a signature of its own
    }
    return hash;
}

void StoredFrames::requireCurrent(const model::Model& model, const results::ResultAttachment& attachment,
                                  base::ThreadPool* pool)
{
    switch (attachment.state(model))
    {
    case results::AttachmentState::Current:
        return;
    case results::AttachmentState::Stale:
        throw base::StaleAttachmentError(
            "the results of '" + attachment.analysis() +
            "' were attached to a model whose structure has changed since (elements or grid points added, removed or "
            "renumbered, or the tables compacted): the links are no longer known to be right; call refresh() on the "
            "attachment (or attach again) to accept the model as it is");
    case results::AttachmentState::Edited:
        if (signature(model, pool) != attachment.frameSignature())
            throw base::StaleAttachmentError(
                "the results of '" + attachment.analysis() + "' were attached to a model whose displacement systems (" +
                cdText(model) + "), the coordinate systems they use or " + materialFrameText(model) +
                " have been edited since: the frames of the results are no longer known to be right; call "
                "refresh() on the attachment (or attach again)");
        return;
    }
}

std::shared_ptr<results::ResultAttachment> attachResults(const std::shared_ptr<model::Model>& model,
                                                         std::shared_ptr<results::ResultFile> file,
                                                         std::shared_ptr<results::ResultTables> tables,
                                                         const results::AttachOptions& options,
                                                         const results::DecodeContext& context, base::ThreadPool* pool)
{
    // attaching again what is attached to a model that has not changed is a no-op (its frames are known)
    if (std::shared_ptr<results::ResultAttachment> same = results::ResultAttachment::unchanged(*model, tables, options))
        return same;
    std::shared_ptr<results::ResultAttachment> attachment =
        results::ResultAttachment::attach(model, std::move(file), std::move(tables), options, context);
    attachment->setFrameSignature(StoredFrames::signature(*model, pool));
    return attachment;
}

void refreshAttachment(const model::Model& model, results::ResultAttachment& attachment, bool strict,
                       const results::DecodeContext& context, base::ThreadPool* pool)
{
    attachment.refresh(model, strict, context);
    attachment.setFrameSignature(StoredFrames::signature(model, pool));
}

} // namespace oor::analysis
