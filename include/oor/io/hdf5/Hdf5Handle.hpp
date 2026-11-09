#pragma once

#include <cstdint>

namespace oor::io
{

// Owner of one libhdf5 identifier (file, group, dataset or attribute): releases it in the destructor, on every path,
// so that an error half-way never leaks an open object. The identifier is kept as a plain 64-bit integer (hid_t is
// one) so that this header, and everything that includes it, needs no HDF5 header: only src/io/hdf5 does.
class Hdf5Handle
{
public:
    Hdf5Handle() noexcept = default;
    explicit Hdf5Handle(std::int64_t identifier) noexcept : m_identifier(identifier) {}
    Hdf5Handle(Hdf5Handle&& other) noexcept : m_identifier(other.m_identifier) { other.m_identifier = INVALID; }
    Hdf5Handle& operator=(Hdf5Handle&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_identifier = other.m_identifier;
            other.m_identifier = INVALID;
        }
        return *this;
    }
    Hdf5Handle(const Hdf5Handle&) = delete;
    Hdf5Handle& operator=(const Hdf5Handle&) = delete;
    ~Hdf5Handle() { reset(); }

    [[nodiscard]] std::int64_t get() const noexcept { return m_identifier; }
    [[nodiscard]] bool isValid() const noexcept { return m_identifier >= 0; }
    void reset() noexcept;

private:
    static constexpr std::int64_t INVALID = -1;
    std::int64_t m_identifier = INVALID;
};

// Number of HDF5 objects (files, groups, datasets, attributes, types) currently open in the process. Zero when
// every handle was released; the tests use it to check that no error path leaks.
[[nodiscard]] std::int64_t openHdf5ObjectCount() noexcept;

} // namespace oor::io
