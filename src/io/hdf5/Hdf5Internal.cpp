#include "Hdf5Internal.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string_view>

namespace oor::io
{

// ------------------------------------------------------------------------------------------------ Hdf5Type text

const char* Hdf5Type::className() const noexcept
{
    switch (typeClass)
    {
    case Hdf5TypeClass::Integer:
        return "integer";
    case Hdf5TypeClass::Float:
        return "float";
    case Hdf5TypeClass::String:
        return "string";
    case Hdf5TypeClass::Compound:
        return "compound";
    case Hdf5TypeClass::Enum:
        return "enum";
    case Hdf5TypeClass::Array:
        return "array";
    case Hdf5TypeClass::VarLen:
        return "vlen";
    case Hdf5TypeClass::Reference:
        return "reference";
    case Hdf5TypeClass::Opaque:
        return "opaque";
    case Hdf5TypeClass::Bitfield:
        return "bitfield";
    case Hdf5TypeClass::Time:
        return "time";
    case Hdf5TypeClass::Other:
        break;
    }
    return "other";
}

namespace
{

std::string dimsText(const std::vector<std::uint64_t>& dims)
{
    std::string text = "[";
    for (std::size_t index = 0; index < dims.size(); ++index)
        text += (index == 0 ? "" : ",") + std::to_string(dims[index]);
    return text + "]";
}

} // namespace

std::string Hdf5Type::description() const
{
    const std::string bigEndian = order == Hdf5Endian::Big ? " BE" : "";
    switch (typeClass)
    {
    case Hdf5TypeClass::Integer:
        return std::string(isSigned ? "int" : "uint") + std::to_string(size * 8) + bigEndian;
    case Hdf5TypeClass::Float:
        return "float" + std::to_string(size * 8) + bigEndian;
    case Hdf5TypeClass::Bitfield:
        return "bitfield" + std::to_string(precision) + bigEndian;
    case Hdf5TypeClass::String:
        return variableLength ? "str" : "S" + std::to_string(size);
    case Hdf5TypeClass::Compound:
    {
        std::string text = "compound{";
        for (std::size_t index = 0; index < members.size(); ++index)
            text += (index == 0 ? "" : ", ") + members[index].name + ": " + members[index].type.description();
        return text + "}";
    }
    case Hdf5TypeClass::Enum:
    {
        std::string text = "enum<" + (base ? base->description() : std::string("?")) + ">{";
        for (std::size_t index = 0; index < enumValues.size(); ++index)
            text += (index == 0 ? "" : ", ") + enumValues[index].name + "=" + std::to_string(enumValues[index].value);
        return text + "}";
    }
    case Hdf5TypeClass::Array:
        return (base ? base->description() : std::string("?")) + dimsText(dims);
    case Hdf5TypeClass::VarLen:
        return "vlen<" + (base ? base->description() : std::string("?")) + ">";
    case Hdf5TypeClass::Reference:
        return "reference[" + std::to_string(size) + "]";
    case Hdf5TypeClass::Opaque:
        return "opaque[" + std::to_string(size) + "]";
    case Hdf5TypeClass::Time:
        return "time";
    case Hdf5TypeClass::Other:
        break;
    }
    return "other";
}

std::string Hdf5Type::numpyString() const
{
    const auto orderCharacter = [this]
    {
        if (size == 1)
            return '|';
        return order == Hdf5Endian::Big ? '>' : '<';
    };
    switch (typeClass)
    {
    case Hdf5TypeClass::Integer:
    case Hdf5TypeClass::Bitfield:
        return std::string(1, orderCharacter()) + (isSigned ? "i" : "u") + std::to_string(size);
    case Hdf5TypeClass::Float:
        return std::string(1, orderCharacter()) + "f" + std::to_string(size);
    case Hdf5TypeClass::String:
        return variableLength ? std::string() : "|S" + std::to_string(size);
    case Hdf5TypeClass::Opaque:
    case Hdf5TypeClass::Reference:
        return "|V" + std::to_string(size);
    case Hdf5TypeClass::Enum:
        return base ? base->numpyString() : std::string();
    default:
        break;
    }
    return {};
}

} // namespace oor::io

namespace oor::io::detail
{

std::mutex& hdf5Mutex()
{
    static std::mutex mutex;
    return mutex;
}

void ensureInitialised()
{
    static const bool SILENCED = []
    {
        H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
        return true;
    }();
    (void)SILENCED;
}

namespace
{

struct ErrorWalk
{
    std::string innermost;
};

herr_t collectError(unsigned, const H5E_error2_t* error, void* data)
{
    auto* walk = static_cast<ErrorWalk*>(data);
    if (walk->innermost.empty() && error->desc != nullptr)
        walk->innermost = error->desc;
    return 0;
}

} // namespace

std::string lastErrorDetail()
{
    ErrorWalk walk;
    H5Ewalk2(H5E_DEFAULT, H5E_WALK_UPWARD, collectError, &walk);
    return walk.innermost;
}

std::string validUtf8(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    std::size_t index = 0;
    while (index < text.size())
    {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::size_t length = 0;
        if (lead < 0x80)
            length = 1;
        else if (lead >= 0xC2 && lead <= 0xDF)
            length = 2;
        else if (lead >= 0xE0 && lead <= 0xEF)
            length = 3;
        else if (lead >= 0xF0 && lead <= 0xF4)
            length = 4;
        bool valid = length != 0 && index + length <= text.size();
        for (std::size_t offset = 1; valid && offset < length; ++offset)
            valid = (static_cast<unsigned char>(text[index + offset]) & 0xC0) == 0x80;
        if (valid && length == 3)
        {
            const auto second = static_cast<unsigned char>(text[index + 1]);
            valid = !(lead == 0xE0 && second < 0xA0) && !(lead == 0xED && second > 0x9F); // overlong, surrogates
        }
        if (valid && length == 4)
        {
            const auto second = static_cast<unsigned char>(text[index + 1]);
            valid = !(lead == 0xF0 && second < 0x90) && !(lead == 0xF4 && second > 0x8F);
        }
        if (valid)
        {
            result.append(text, index, length);
            index += length;
        }
        else
        {
            result += "ï¿½";
            ++index;
        }
    }
    return result;
}

void fail(const std::string& what)
{
    ensureInitialised();
    const std::string detail = lastErrorDetail();
    throw base::IoError(validUtf8("HDF5: " + what + (detail.empty() ? std::string() : " (" + detail + ")")));
}

Hdf5Handle checked(hid_t identifier, const std::string& what)
{
    if (identifier < 0)
        fail(what);
    return Hdf5Handle(identifier);
}

std::uint64_t checkedProduct(std::uint64_t first, std::uint64_t second, const std::string& what)
{
    if (first != 0 && second > std::numeric_limits<std::uint64_t>::max() / first)
        throw base::LimitExceededError(what + ": the size does not fit in 64 bits");
    return first * second;
}

std::uint64_t checkedProduct(const std::vector<std::uint64_t>& values, const std::string& what)
{
    std::uint64_t product = 1;
    for (const std::uint64_t value : values)
        product = checkedProduct(product, value, what);
    return product;
}

// ------------------------------------------------------------------------------------------------ type description

namespace
{

Hdf5Endian endianOf(hid_t type)
{
    if (H5Tget_size(type) == 1)
        return Hdf5Endian::None;
    switch (H5Tget_order(type))
    {
    case H5T_ORDER_LE:
        return Hdf5Endian::Little;
    case H5T_ORDER_BE:
        return Hdf5Endian::Big;
    default:
        return Hdf5Endian::None;
    }
}

std::string ownedName(char* name)
{
    if (name == nullptr)
        return {};
    std::string text(name);
    H5free_memory(name);
    return text;
}

std::int64_t enumValueOf(hid_t type, unsigned index, const Hdf5Type& base)
{
    std::uint64_t raw = 0;
    if (H5Tget_member_value(type, index, &raw) < 0)
        fail("cannot read an enum value");
    const std::size_t size = std::min<std::size_t>(std::max<std::size_t>(base.size, 1), sizeof(raw));
    if (base.isSigned)
    {
        const unsigned bits = static_cast<unsigned>(size * 8);
        if (bits < 64 && (raw & (std::uint64_t{1} << (bits - 1))) != 0)
            raw |= ~std::uint64_t{0} << bits;
    }
    else if (size < sizeof(raw))
    {
        raw &= (std::uint64_t{1} << (size * 8)) - 1;
    }
    return static_cast<std::int64_t>(raw);
}

} // namespace

Hdf5Type describeType(hid_t type, const Hdf5Limits& limits, std::uint32_t depth)
{
    if (depth > limits.maxTypeDepth)
        throw base::LimitExceededError("HDF5: data types nest deeper than " + std::to_string(limits.maxTypeDepth));
    Hdf5Type result;
    result.size = H5Tget_size(type);
    switch (H5Tget_class(type))
    {
    case H5T_INTEGER:
        result.typeClass = Hdf5TypeClass::Integer;
        result.isSigned = H5Tget_sign(type) == H5T_SGN_2;
        result.order = endianOf(type);
        result.precision = H5Tget_precision(type);
        break;
    case H5T_FLOAT:
        result.typeClass = Hdf5TypeClass::Float;
        result.isSigned = true;
        result.order = endianOf(type);
        result.precision = H5Tget_precision(type);
        break;
    case H5T_BITFIELD:
        result.typeClass = Hdf5TypeClass::Bitfield;
        result.order = endianOf(type);
        result.precision = H5Tget_precision(type);
        break;
    case H5T_TIME:
        result.typeClass = Hdf5TypeClass::Time;
        break;
    case H5T_STRING:
    {
        result.typeClass = Hdf5TypeClass::String;
        result.variableLength = H5Tis_variable_str(type) > 0;
        result.utf8 = H5Tget_cset(type) == H5T_CSET_UTF8;
        switch (H5Tget_strpad(type))
        {
        case H5T_STR_NULLTERM:
            result.padding = "nullterm";
            break;
        case H5T_STR_NULLPAD:
            result.padding = "nullpad";
            break;
        default:
            result.padding = "spacepad";
            break;
        }
        if (result.variableLength)
            result.size = 0;
        break;
    }
    case H5T_OPAQUE:
        result.typeClass = Hdf5TypeClass::Opaque;
        result.tag = ownedName(H5Tget_tag(type));
        break;
    case H5T_REFERENCE:
        result.typeClass = Hdf5TypeClass::Reference;
        break;
    case H5T_COMPOUND:
    {
        result.typeClass = Hdf5TypeClass::Compound;
        const int count = H5Tget_nmembers(type);
        if (count < 0)
            fail("cannot count the members of a compound type");
        if (static_cast<std::uint64_t>(count) > limits.maxCompoundMembers)
            throw base::LimitExceededError("HDF5: a compound type has " + std::to_string(count) +
                                           " members (the limit is " + std::to_string(limits.maxCompoundMembers) + ")");
        result.members.reserve(static_cast<std::size_t>(count));
        for (int index = 0; index < count; ++index)
        {
            Hdf5Type::Member member;
            member.name = ownedName(H5Tget_member_name(type, static_cast<unsigned>(index)));
            member.offset = H5Tget_member_offset(type, static_cast<unsigned>(index));
            const Hdf5Handle memberType = checked(H5Tget_member_type(type, static_cast<unsigned>(index)),
                                                  "cannot read a member of a compound type");
            member.type = describeType(id(memberType), limits, depth + 1);
            result.members.push_back(std::move(member));
        }
        break;
    }
    case H5T_ENUM:
    {
        result.typeClass = Hdf5TypeClass::Enum;
        const Hdf5Handle super = checked(H5Tget_super(type), "cannot read the base of an enum type");
        auto base = std::make_shared<Hdf5Type>(describeType(id(super), limits, depth + 1));
        result.isSigned = base->isSigned;
        result.order = base->order;
        result.precision = base->precision;
        const int count = H5Tget_nmembers(type);
        if (count < 0)
            fail("cannot count the values of an enum type");
        if (static_cast<std::uint64_t>(count) > limits.maxCompoundMembers)
            throw base::LimitExceededError("HDF5: an enum type has " + std::to_string(count) +
                                           " values (the limit is " + std::to_string(limits.maxCompoundMembers) + ")");
        for (int index = 0; index < count; ++index)
        {
            Hdf5Type::EnumValue value;
            value.name = ownedName(H5Tget_member_name(type, static_cast<unsigned>(index)));
            value.value = enumValueOf(type, static_cast<unsigned>(index), *base);
            result.enumValues.push_back(std::move(value));
        }
        result.base = std::move(base);
        break;
    }
    case H5T_ARRAY:
    {
        result.typeClass = Hdf5TypeClass::Array;
        const int rank = H5Tget_array_ndims(type);
        if (rank < 0)
            fail("cannot read the rank of an array type");
        std::vector<hsize_t> dims(static_cast<std::size_t>(rank));
        if (rank > 0 && H5Tget_array_dims2(type, dims.data()) < 0)
            fail("cannot read the dimensions of an array type");
        result.dims.assign(dims.begin(), dims.end());
        const Hdf5Handle super = checked(H5Tget_super(type), "cannot read the base of an array type");
        result.base = std::make_shared<Hdf5Type>(describeType(id(super), limits, depth + 1));
        break;
    }
    case H5T_VLEN:
    {
        result.typeClass = Hdf5TypeClass::VarLen;
        result.size = 0;
        const Hdf5Handle super = checked(H5Tget_super(type), "cannot read the base of a variable-length type");
        result.base = std::make_shared<Hdf5Type>(describeType(id(super), limits, depth + 1));
        break;
    }
    default:
        result.typeClass = Hdf5TypeClass::Other;
        break;
    }
    return result;
}

// ------------------------------------------------------------------------------------------------ links

namespace
{

constexpr int MAX_SOFT_LINK_HOPS = 16;

std::vector<std::string> splitPath(std::string_view path)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= path.size())
    {
        const std::size_t slash = path.find('/', start);
        const std::string_view part =
            path.substr(start, slash == std::string_view::npos ? std::string_view::npos : slash - start);
        if (!part.empty() && part != ".")
            parts.emplace_back(part);
        if (slash == std::string_view::npos)
            break;
        start = slash + 1;
    }
    return parts;
}

void refuseExternalLinksImpl(hid_t base, const std::string& path, int hops)
{
    if (hops > MAX_SOFT_LINK_HOPS)
        throw base::IoError("HDF5: more than " + std::to_string(MAX_SOFT_LINK_HOPS) + " soft links on the path " +
                            path);
    const std::vector<std::string> parts = splitPath(path);
    std::string prefix = !path.empty() && path.front() == '/' ? "/" : "";
    std::string parent = prefix;
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
        const std::string current = (prefix.empty() || prefix == "/" ? prefix : prefix + "/") + parts[index];
        H5L_info2_t info;
        if (H5Lget_info2(base, current.c_str(), &info, H5P_DEFAULT) < 0)
            return; // missing: the open that follows says so
        if (info.type == H5L_TYPE_EXTERNAL)
            throw base::IoError("HDF5: the path " + path + " goes through an external link, which is never followed");
        if (info.type == H5L_TYPE_SOFT)
        {
            std::string target(info.u.val_size, '\0');
            if (H5Lget_val(base, current.c_str(), target.data(), target.size(), H5P_DEFAULT) < 0)
                return;
            target.erase(std::find(target.begin(), target.end(), '\0'), target.end());
            std::string resolved = target;
            if (target.empty() || target.front() != '/')
                resolved = (parent.empty() ? std::string() : (parent == "/" ? parent : parent + "/")) + target;
            for (std::size_t rest = index + 1; rest < parts.size(); ++rest)
                resolved += "/" + parts[rest];
            refuseExternalLinksImpl(base, resolved, hops + 1);
            return;
        }
        parent = current;
        prefix = current;
    }
}

} // namespace

void refuseExternalLinks(hid_t base, const std::string& path)
{
    refuseExternalLinksImpl(base, path, 0);
}

Hdf5Handle openFileReadOnly(const std::string& utf8Path)
{
    ensureInitialised();
    return checked(H5Fopen(utf8Path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT), "cannot open HDF5 file: " + utf8Path);
}

} // namespace oor::io::detail
