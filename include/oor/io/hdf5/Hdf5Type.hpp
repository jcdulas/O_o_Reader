#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace oor::io
{

enum class Hdf5TypeClass
{
    Integer,
    Float,
    String,
    Compound,
    Other,
    // The classes below were added with the generic reader; Other keeps its place for the code that already switched
    // on the first five.
    Enum,
    Array,
    VarLen, // a variable-length sequence of a base type (not a string)
    Reference,
    Opaque,
    Bitfield,
    Time
};

enum class Hdf5Endian
{
    Little,
    Big,
    None // one byte, a string, or a type with no byte order
};

// The description of one data type of a file: what the type is, never any data. Built by the tree walk and by the
// table reader from the libhdf5 type; the Python class `H5Type` mirrors it.
struct Hdf5Type
{
    struct Member;
    struct EnumValue;

    Hdf5TypeClass typeClass = Hdf5TypeClass::Other;
    // Bytes of one element in the file. Zero for a variable-length string or sequence (the element is a pointer).
    std::size_t size = 0;
    // Integer, enum base, bitfield: signed or not. Float: always true.
    bool isSigned = false;
    Hdf5Endian order = Hdf5Endian::None;
    // Bits of precision (integer, float, bitfield).
    std::size_t precision = 0;
    // String: variable length, character set UTF-8 (else ASCII), padding ("nullterm", "nullpad" or "spacepad").
    bool variableLength = false;
    bool utf8 = false;
    std::string padding;
    // Compound members, in the order of the file.
    std::vector<Member> members;
    // Enum names and values.
    std::vector<EnumValue> enumValues;
    // Array dimensions; `base` is the type of the element of an array, of a variable-length sequence, and the integer
    // type of an enum.
    std::vector<std::uint64_t> dims;
    std::shared_ptr<const Hdf5Type> base;
    // Opaque: the tag the file gives it.
    std::string tag;

    // "integer", "float", "string", "compound", "enum", "array", "vlen", "reference", "opaque", "bitfield", "time",
    // "other".
    [[nodiscard]] const char* className() const noexcept;
    // A short readable form: "int64", "uint8", "float32", "S8" (fixed string of 8 bytes), "str" (variable length),
    // "compound{DOMAIN_ID:int64, ...}", "float64[5]", "enum<int8>{FALSE=0, TRUE=1}", "vlen<int32>", "opaque[16]".
    [[nodiscard]] std::string description() const;
    // The NumPy type string of a type that has one ("<i8", "|S8", "<f4"), empty for the others (compound, variable
    // length, ...): the Python layer builds structured dtypes from the members.
    [[nodiscard]] std::string numpyString() const;
};

struct Hdf5Type::Member
{
    std::string name;
    std::size_t offset = 0;
    Hdf5Type type;
};

struct Hdf5Type::EnumValue
{
    std::string name;
    std::int64_t value = 0;
};

} // namespace oor::io
