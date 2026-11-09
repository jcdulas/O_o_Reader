#include "oor/io/hdf5/Hdf5Tree.hpp"

#include "Hdf5Internal.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/io/hdf5/Hdf5GlobalHeapCheck.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>

namespace oor::io
{

using detail::checked;
using detail::checkedProduct;
using detail::fail;
using detail::id;

const char* nodeKindName(Hdf5NodeKind kind) noexcept
{
    switch (kind)
    {
    case Hdf5NodeKind::Group:
        return "group";
    case Hdf5NodeKind::Dataset:
        return "dataset";
    case Hdf5NodeKind::NamedType:
        return "named_type";
    case Hdf5NodeKind::SoftLink:
        return "soft_link";
    case Hdf5NodeKind::ExternalLink:
        return "external_link";
    case Hdf5NodeKind::Other:
        break;
    }
    return "other";
}

std::uint64_t Hdf5Attribute::elementCount() const noexcept
{
    if (isNull)
        return 0;
    std::uint64_t count = 1;
    for (const std::uint64_t extent : shape)
        count *= extent;
    return count;
}

std::uint64_t Hdf5Node::elementCount() const noexcept
{
    if (isNull)
        return 0;
    std::uint64_t count = 1;
    for (const std::uint64_t extent : shape)
        count *= extent;
    return count;
}

const Hdf5Attribute* Hdf5Node::attribute(const std::string& attributeName) const noexcept
{
    for (const Hdf5Attribute& candidate : attributes)
        if (candidate.name == attributeName)
            return &candidate;
    return nullptr;
}

const Hdf5Node* Hdf5Tree::find(const std::string& path) const
{
    const auto found = m_index.find(path);
    return found == m_index.end() ? nullptr : &m_nodes[found->second];
}

std::uint64_t Hdf5Tree::datasetCount() const noexcept
{
    return static_cast<std::uint64_t>(std::count_if(
        m_nodes.begin(), m_nodes.end(), [](const Hdf5Node& node) { return node.kind == Hdf5NodeKind::Dataset; }));
}

namespace
{

// ------------------------------------------------------------------------------------------------ attributes

// The check of the global heap of the file (Hdf5GlobalHeapCheck.hpp), made once, the first time the walk is about to
// read a variable-length value: a damaged heap would make libhdf5 loop forever in H5Aread.
class GlobalHeapGate
{
public:
    explicit GlobalHeapGate(std::filesystem::path file) : m_file(std::move(file)) {}

    [[nodiscard]] const std::optional<std::string>& problem()
    {
        if (!m_checked)
        {
            m_problem = findGlobalHeapProblem(m_file);
            m_checked = true;
        }
        return m_problem;
    }

private:
    std::filesystem::path m_file;
    bool m_checked = false;
    std::optional<std::string> m_problem;
};

std::string trimmedString(const char* data, std::size_t size, const std::string& padding)
{
    std::size_t length = size;
    if (padding == "spacepad")
    {
        while (length > 0 && (data[length - 1] == ' ' || data[length - 1] == '\0'))
            --length;
    }
    else
    {
        length = static_cast<std::size_t>(std::find(data, data + size, '\0') - data);
    }
    return std::string(data, length);
}

// Dimensions of a dataspace: empty for a scalar. `isNull` for an empty dataspace.
std::vector<std::uint64_t> spaceDims(hid_t space, bool& isNull, std::vector<std::uint64_t>* maxDims = nullptr)
{
    isNull = false;
    const H5S_class_t spaceClass = H5Sget_simple_extent_type(space);
    if (spaceClass == H5S_NULL)
    {
        isNull = true;
        return {};
    }
    if (spaceClass == H5S_SCALAR)
        return {};
    const int rank = H5Sget_simple_extent_ndims(space);
    if (rank < 0)
        fail("cannot get the rank of a dataspace");
    std::vector<hsize_t> dims(static_cast<std::size_t>(rank));
    std::vector<hsize_t> maxima(static_cast<std::size_t>(rank));
    if (rank > 0 && H5Sget_simple_extent_dims(space, dims.data(), maxima.data()) < 0)
        fail("cannot get the dimensions of a dataspace");
    if (maxDims != nullptr)
        maxDims->assign(maxima.begin(), maxima.end());
    return std::vector<std::uint64_t>(dims.begin(), dims.end());
}

void readAttributeValue(Hdf5Attribute& attribute, hid_t attributeId, hid_t fileType, const Hdf5Limits& limits,
                        GlobalHeapGate& heap)
{
    if (attribute.isNull)
    {
        attribute.unreadReason = "empty dataspace";
        return;
    }
    // The element type: an array type is read as its base with the array dimensions added to the shape.
    const Hdf5Type* element = &attribute.type;
    Hdf5Handle baseType;
    std::uint64_t perElement = 1;
    if (element->typeClass == Hdf5TypeClass::Array && element->base)
    {
        perElement = checkedProduct(element->dims, "an attribute");
        element = element->base.get();
        baseType = checked(H5Tget_super(fileType), "cannot read the base of an attribute type");
    }
    const hid_t elementType = baseType.isValid() ? id(baseType) : fileType;
    const std::uint64_t count = checkedProduct(attribute.elementCount(), perElement, "an attribute");
    if (!attribute.type.dims.empty() && element != &attribute.type)
        attribute.shape.insert(attribute.shape.end(), attribute.type.dims.begin(), attribute.type.dims.end());

    switch (element->typeClass)
    {
    case Hdf5TypeClass::Integer:
    case Hdf5TypeClass::Bitfield:
    case Hdf5TypeClass::Enum:
        // the library converts these value by value: a damaged file can claim an integer of a megabyte, and the
        // conversion of one takes a very long time
        if (element->size > 8)
        {
            attribute.unreadReason = "integer wider than 64 bits";
            return;
        }
        break;
    case Hdf5TypeClass::Float:
        if (element->size > 16)
        {
            attribute.unreadReason = "float wider than 128 bits";
            return;
        }
        break;
    case Hdf5TypeClass::String:
        if (element->variableLength && !limits.readVarLenAttributes)
        {
            attribute.unreadReason = "variable-length strings are not read unless asked for (vlen_attributes)";
            return;
        }
        if (element->variableLength && heap.problem())
        {
            attribute.unreadReason = "the global heap of the file is damaged: " + *heap.problem();
            return;
        }
        break;
    default:
        attribute.unreadReason = std::string(element->className()) + " type";
        return;
    }
    const std::uint64_t elementBytes =
        element->variableLength ? sizeof(char*) : std::max<std::size_t>(element->size, 1);
    if (checkedProduct(count, elementBytes, "an attribute") > limits.maxAttributeBytes)
    {
        attribute.unreadReason = "larger than the limit of " + std::to_string(limits.maxAttributeBytes) + " bytes";
        return;
    }
    const std::size_t n = static_cast<std::size_t>(count);
    switch (element->typeClass)
    {
    case Hdf5TypeClass::Integer:
    case Hdf5TypeClass::Bitfield:
    case Hdf5TypeClass::Enum:
    {
        if (element->isSigned && element->typeClass != Hdf5TypeClass::Bitfield)
        {
            std::vector<std::int64_t> values(n);
            if (n > 0 && H5Aread(attributeId, H5T_NATIVE_INT64, values.data()) < 0)
                fail("cannot read attribute " + attribute.name);
            attribute.value = std::move(values);
        }
        else
        {
            std::vector<std::uint64_t> values(n);
            if (n > 0 && H5Aread(attributeId, H5T_NATIVE_UINT64, values.data()) < 0)
                fail("cannot read attribute " + attribute.name);
            attribute.value = std::move(values);
        }
        return;
    }
    case Hdf5TypeClass::Float:
    {
        std::vector<double> values(n);
        if (n > 0 && H5Aread(attributeId, H5T_NATIVE_DOUBLE, values.data()) < 0)
            fail("cannot read attribute " + attribute.name);
        attribute.value = std::move(values);
        return;
    }
    case Hdf5TypeClass::String:
    {
        std::vector<std::string> values;
        values.reserve(n);
        if (element->variableLength)
        {
            Hdf5Handle memoryType = checked(H5Tcopy(H5T_C_S1), "cannot create a string type");
            H5Tset_size(id(memoryType), H5T_VARIABLE);
            H5Tset_cset(id(memoryType), element->utf8 ? H5T_CSET_UTF8 : H5T_CSET_ASCII);
            std::vector<char*> pointers(n, nullptr);
            if (n > 0 && H5Aread(attributeId, id(memoryType), pointers.data()) < 0)
                fail("cannot read attribute " + attribute.name);
            for (const char* pointer : pointers)
            {
                if (pointer == nullptr)
                {
                    values.emplace_back();
                    continue;
                }
                const std::size_t length = ::strnlen(pointer, static_cast<std::size_t>(limits.maxStringBytes));
                values.emplace_back(pointer, length);
            }
            if (n > 0)
            {
                const Hdf5Handle space = checked(H5Aget_space(attributeId), "cannot get the dataspace of an attribute");
                H5Treclaim(id(memoryType), id(space), H5P_DEFAULT, pointers.data());
            }
        }
        else
        {
            const std::size_t size = element->size;
            std::vector<char> raw(n * size);
            if (n > 0 && H5Aread(attributeId, elementType, raw.data()) < 0)
                fail("cannot read attribute " + attribute.name);
            for (std::size_t index = 0; index < n; ++index)
                values.push_back(trimmedString(raw.data() + index * size, size, element->padding));
        }
        attribute.value = std::move(values);
        return;
    }
    default:
        return;
    }
}

void readAttributes(hid_t object, const std::string& where, Hdf5Node& node, const Hdf5Limits& limits,
                    GlobalHeapGate& heap)
{
    H5O_info2_t info;
    if (H5Oget_info3(object, &info, H5O_INFO_NUM_ATTRS) < 0)
        fail("cannot count the attributes of " + where);
    if (info.num_attrs > limits.maxAttributesPerObject)
        throw base::LimitExceededError("HDF5: " + where + " has " + std::to_string(info.num_attrs) +
                                       " attributes (the limit is " + std::to_string(limits.maxAttributesPerObject) +
                                       ")");
    for (hsize_t index = 0; index < info.num_attrs; ++index)
    {
        const Hdf5Handle attributeId =
            checked(H5Aopen_by_idx(object, ".", H5_INDEX_NAME, H5_ITER_INC, index, H5P_DEFAULT, H5P_DEFAULT),
                    "cannot open an attribute of " + where);
        Hdf5Attribute attribute;
        const ssize_t nameLength = H5Aget_name(id(attributeId), 0, nullptr);
        if (nameLength < 0)
            fail("cannot read the name of an attribute of " + where);
        attribute.name.assign(static_cast<std::size_t>(nameLength) + 1, '\0');
        H5Aget_name(id(attributeId), attribute.name.size(), attribute.name.data());
        attribute.name.resize(static_cast<std::size_t>(nameLength));
        const Hdf5Handle type =
            checked(H5Aget_type(id(attributeId)), "cannot read the type of attribute " + attribute.name);
        attribute.type = detail::describeType(id(type), limits);
        const Hdf5Handle space =
            checked(H5Aget_space(id(attributeId)), "cannot read the shape of attribute " + attribute.name);
        attribute.shape = spaceDims(id(space), attribute.isNull);
        readAttributeValue(attribute, id(attributeId), id(type), limits, heap);
        node.attributes.push_back(std::move(attribute));
    }
}

// ------------------------------------------------------------------------------------------------ datasets

std::string filterName(H5Z_filter_t filter, const char* given)
{
    switch (filter)
    {
    case H5Z_FILTER_DEFLATE:
        return "deflate";
    case H5Z_FILTER_SHUFFLE:
        return "shuffle";
    case H5Z_FILTER_FLETCHER32:
        return "fletcher32";
    case H5Z_FILTER_SZIP:
        return "szip";
    case H5Z_FILTER_NBIT:
        return "nbit";
    case H5Z_FILTER_SCALEOFFSET:
        return "scaleoffset";
    default:
        break;
    }
    if (given != nullptr && given[0] != '\0')
        return given;
    return "filter " + std::to_string(static_cast<int>(filter));
}

void describeDataset(hid_t dataset, const std::string& where, Hdf5Node& node, const Hdf5Limits& limits)
{
    const Hdf5Handle type = checked(H5Dget_type(dataset), "cannot read the type of " + where);
    node.type = detail::describeType(id(type), limits);
    const Hdf5Handle space = checked(H5Dget_space(dataset), "cannot read the shape of " + where);
    node.shape = spaceDims(id(space), node.isNull, &node.maxShape);
    const Hdf5Handle plist = checked(H5Dget_create_plist(dataset), "cannot read the properties of " + where);
    switch (H5Pget_layout(id(plist)))
    {
    case H5D_COMPACT:
        node.layout = "compact";
        break;
    case H5D_CONTIGUOUS:
        node.layout = "contiguous";
        break;
    case H5D_CHUNKED:
    {
        node.layout = "chunked";
        std::vector<hsize_t> chunks(std::max<std::size_t>(node.shape.size(), 1) + 1);
        const int rank = H5Pget_chunk(id(plist), static_cast<int>(chunks.size()), chunks.data());
        if (rank < 0)
            fail("cannot read the chunk shape of " + where);
        chunks.resize(std::min<std::size_t>(static_cast<std::size_t>(rank), chunks.size()));
        node.chunks.assign(chunks.begin(), chunks.end());
        break;
    }
    case H5D_VIRTUAL:
        node.layout = "virtual";
        break;
    default:
        node.layout = "other";
        break;
    }
    const int filterCount = H5Pget_nfilters(id(plist));
    for (int index = 0; index < std::max(filterCount, 0); ++index)
    {
        unsigned flags = 0;
        size_t parameterCount = 16;
        unsigned parameters[16] = {};
        char name[64] = {};
        unsigned config = 0;
        const H5Z_filter_t filter = H5Pget_filter2(id(plist), static_cast<unsigned>(index), &flags, &parameterCount,
                                                   parameters, sizeof(name), name, &config);
        if (filter < 0)
            fail("cannot read a filter of " + where);
        Hdf5Filter entry;
        entry.id = static_cast<std::uint32_t>(filter);
        entry.name = filterName(filter, name);
        entry.optional = (flags & H5Z_FLAG_OPTIONAL) != 0;
        entry.parameters.assign(parameters, parameters + std::min<size_t>(parameterCount, 16));
        node.filters.push_back(std::move(entry));
    }
    node.storageBytes = H5Dget_storage_size(dataset);
}

// ------------------------------------------------------------------------------------------------ the walk

std::string objectKey(const H5O_info2_t& info)
{
    std::string key(reinterpret_cast<const char*>(&info.token), sizeof(info.token));
    key += ':' + std::to_string(info.fileno);
    return key;
}

struct ChildLink
{
    std::string name;
    H5L_type_t type = H5L_TYPE_ERROR;
};

struct ChildCollector
{
    std::vector<ChildLink> links;
    std::uint64_t budget = 0; // how many more objects the limit allows
};

herr_t collectChild(hid_t, const char* name, const H5L_info2_t* info, void* data)
{
    auto* collector = static_cast<ChildCollector*>(data);
    if (collector->links.size() >= collector->budget)
        return 1; // stops the iteration: the walker reports the limit
    collector->links.push_back(ChildLink{name, info->type});
    return 0;
}

class Walker
{
public:
    Walker(const Hdf5Limits& limits, const base::CancellationToken* token, base::ProgressCounter* progress,
           std::vector<Hdf5Node>& nodes, const std::filesystem::path& file)
        : m_limits(limits), m_token(token), m_progress(progress), m_nodes(nodes), m_heap(file)
    {
    }

    void run(hid_t root)
    {
        Hdf5Node rootNode;
        rootNode.path = "/";
        rootNode.kind = Hdf5NodeKind::Group;
        m_nodes.push_back(std::move(rootNode));
        H5O_info2_t info;
        if (H5Oget_info3(root, &info, H5O_INFO_BASIC) < 0)
            fail("cannot read the root group");
        m_visited.emplace(objectKey(info), "/");
        readAttributes(root, "the root group", m_nodes[0], m_limits, m_heap);
        walkGroup(root, 0);
    }

private:
    void walkGroup(hid_t group, std::uint32_t groupIndex)
    {
        const std::uint32_t depth = m_nodes[groupIndex].depth;
        if (depth >= m_limits.maxDepth)
            throw base::LimitExceededError("HDF5: groups nest deeper than " + std::to_string(m_limits.maxDepth) + " (" +
                                           m_nodes[groupIndex].path + ")");
        ChildCollector collector;
        collector.budget = m_limits.maxObjects > m_nodes.size() ? m_limits.maxObjects - m_nodes.size() : 0;
        hsize_t position = 0;
        const herr_t result = H5Literate2(group, H5_INDEX_NAME, H5_ITER_INC, &position, collectChild, &collector);
        if (result > 0)
            throw base::LimitExceededError("HDF5: the file has more than " + std::to_string(m_limits.maxObjects) +
                                           " objects");
        if (result < 0)
            fail("cannot list the children of " + m_nodes[groupIndex].path);

        const std::string parentPath = m_nodes[groupIndex].path;
        std::vector<std::uint32_t> childIndices;
        childIndices.reserve(collector.links.size());
        for (const ChildLink& link : collector.links)
        {
            if (m_token != nullptr)
                m_token->throwIfCancelled();
            Hdf5Node node;
            node.name = link.name;
            node.path = (parentPath == "/" ? std::string() : parentPath) + "/" + link.name;
            if (node.path.size() > m_limits.maxPathBytes)
                throw base::LimitExceededError("HDF5: an object path is longer than " +
                                               std::to_string(m_limits.maxPathBytes) + " bytes");
            node.depth = depth + 1;
            node.parent = groupIndex;
            if (m_nodes.size() >= m_limits.maxObjects)
                throw base::LimitExceededError("HDF5: the file has more than " + std::to_string(m_limits.maxObjects) +
                                               " objects");
            const std::uint32_t index = static_cast<std::uint32_t>(m_nodes.size());
            switch (link.type)
            {
            case H5L_TYPE_SOFT:
                node.kind = Hdf5NodeKind::SoftLink;
                node.linkTarget = softLinkTarget(group, link.name);
                m_nodes.push_back(std::move(node));
                break;
            case H5L_TYPE_EXTERNAL:
                node.kind = Hdf5NodeKind::ExternalLink;
                node.linkTarget = externalLinkTarget(group, link.name);
                m_nodes.push_back(std::move(node));
                break;
            case H5L_TYPE_HARD:
                m_nodes.push_back(std::move(node));
                hardLink(group, index);
                break;
            default:
                node.kind = Hdf5NodeKind::Other;
                m_nodes.push_back(std::move(node));
                break;
            }
            childIndices.push_back(index);
            if (m_progress != nullptr)
                m_progress->add(1);
        }
        m_nodes[groupIndex].children = std::move(childIndices);
    }

    std::string softLinkTarget(hid_t group, const std::string& name)
    {
        H5L_info2_t info;
        if (H5Lget_info2(group, name.c_str(), &info, H5P_DEFAULT) < 0)
            fail("cannot read the link " + name);
        std::string target(info.u.val_size, '\0');
        if (!target.empty() && H5Lget_val(group, name.c_str(), target.data(), target.size(), H5P_DEFAULT) < 0)
            fail("cannot read the link " + name);
        target.erase(std::find(target.begin(), target.end(), '\0'), target.end());
        return target;
    }

    std::string externalLinkTarget(hid_t group, const std::string& name)
    {
        H5L_info2_t info;
        if (H5Lget_info2(group, name.c_str(), &info, H5P_DEFAULT) < 0)
            fail("cannot read the link " + name);
        std::vector<char> buffer(info.u.val_size + 1, '\0');
        if (H5Lget_val(group, name.c_str(), buffer.data(), info.u.val_size, H5P_DEFAULT) < 0)
            fail("cannot read the link " + name);
        const char* file = nullptr;
        const char* object = nullptr;
        unsigned flags = 0;
        if (H5Lunpack_elink_val(buffer.data(), info.u.val_size, &flags, &file, &object) < 0)
            fail("cannot read the external link " + name);
        return std::string(file != nullptr ? file : "") + "::" + (object != nullptr ? object : "");
    }

    void hardLink(hid_t group, std::uint32_t index)
    {
        const std::string name = m_nodes[index].name;
        const std::string path = m_nodes[index].path;
        H5O_info2_t info;
        if (H5Oget_info_by_name3(group, name.c_str(), &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
            fail("cannot read the object " + path);
        switch (info.type)
        {
        case H5O_TYPE_GROUP:
            m_nodes[index].kind = Hdf5NodeKind::Group;
            break;
        case H5O_TYPE_DATASET:
            m_nodes[index].kind = Hdf5NodeKind::Dataset;
            break;
        case H5O_TYPE_NAMED_DATATYPE:
            m_nodes[index].kind = Hdf5NodeKind::NamedType;
            break;
        default:
            m_nodes[index].kind = Hdf5NodeKind::Other;
            return;
        }
        const auto [position, inserted] = m_visited.emplace(objectKey(info), path);
        if (!inserted)
        {
            m_nodes[index].aliasOf = position->second;
            return;
        }
        switch (info.type)
        {
        case H5O_TYPE_GROUP:
        {
            const Hdf5Handle opened =
                checked(H5Gopen2(group, name.c_str(), H5P_DEFAULT), "cannot open the group " + path);
            readAttributes(id(opened), path, m_nodes[index], m_limits, m_heap);
            walkGroup(id(opened), index);
            break;
        }
        case H5O_TYPE_DATASET:
        {
            const Hdf5Handle opened =
                checked(H5Dopen2(group, name.c_str(), H5P_DEFAULT), "cannot open the dataset " + path);
            describeDataset(id(opened), path, m_nodes[index], m_limits);
            readAttributes(id(opened), path, m_nodes[index], m_limits, m_heap);
            break;
        }
        default:
        {
            const Hdf5Handle opened =
                checked(H5Topen2(group, name.c_str(), H5P_DEFAULT), "cannot open the type " + path);
            m_nodes[index].type = detail::describeType(id(opened), m_limits);
            readAttributes(id(opened), path, m_nodes[index], m_limits, m_heap);
            break;
        }
        }
    }

    const Hdf5Limits& m_limits;
    const base::CancellationToken* m_token;
    base::ProgressCounter* m_progress;
    std::vector<Hdf5Node>& m_nodes;
    GlobalHeapGate m_heap;
    std::unordered_map<std::string, std::string> m_visited;
};

} // namespace

Hdf5Tree Hdf5Tree::read(const std::filesystem::path& file, const Hdf5Limits& limits,
                        const base::CancellationToken* token, base::ProgressCounter* progress)
{
    return detail::cleanErrors(
        [&]
        {
            const std::lock_guard<std::mutex> lock(detail::hdf5Mutex());
            detail::ensureInitialised();
            if (progress != nullptr)
                progress->beginStage("h5_tree", 0);
            const Hdf5Handle opened = detail::openFileReadOnly(base::pathToUtf8(file));
            const Hdf5Handle root = checked(H5Gopen2(id(opened), "/", H5P_DEFAULT), "cannot open the root group");
            Hdf5Tree tree;
            Walker walker(limits, token, progress, tree.m_nodes, file);
            walker.run(id(root));
            for (std::size_t index = 0; index < tree.m_nodes.size(); ++index)
                tree.m_index.emplace(tree.m_nodes[index].path, static_cast<std::uint32_t>(index));
            return tree;
        });
}

} // namespace oor::io
