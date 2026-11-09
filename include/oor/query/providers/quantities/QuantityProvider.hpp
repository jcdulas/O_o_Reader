#pragma once

#include "oor/query/results/ResultSets.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// What the catalogue of the quantities says of one way of obtaining a quantity.
struct ProviderInfo
{
    std::string name;
    bool approximate = false;
};

// A quantity and the ways of obtaining it, in the order they are tried.
struct QuantityInfo
{
    std::string quantity;
    std::vector<ProviderInfo> providers;
};

// The ways of obtaining a quantity that tests (and users who want to know what a recomputation gives) can switch off by
// name: a disabled provider is skipped and says so in the error that lists the ways tried. The list is global to the
// process.
class ProviderSwitches
{
public:
    [[nodiscard]] static bool isDisabled(const std::string& name);
    static void setDisabled(std::vector<std::string> names);
    [[nodiscard]] static std::vector<std::string> disabled();
};

// One way of obtaining a quantity (source resolution): a class of its own, written by hand. It is
// given the request (what the query wants: the elements, the subcases, the components) and a sink that holds what the
// ways before it gave; it adds the rows it can give that the sink does not have yet (per-row resolution: the sink drops
// a row it has already) and says how many, or why none. A provider never throws because it cannot give a quantity: it
// says so.
template <typename Request, typename Sink>
class QuantityProvider
{
public:
    struct Outcome
    {
        std::int64_t rows = 0;
        std::string reason; // why it gave nothing (empty when it gave rows)
    };

    virtual ~QuantityProvider() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    // True for a way that gives an estimate and not the quantity (an integration of stresses, a first order
    // prediction): it is tried last and marked.
    [[nodiscard]] virtual bool approximate() const { return false; }
    [[nodiscard]] virtual Outcome provide(const ResultQueryContext& context, const Request& request,
                                          Sink& sink) const = 0;
};

// The ordered providers of one quantity. The sink must have `void beginProvider(std::int16_t index, const
// ProviderInfo&)` (the chain says which provider is about to add rows) and `std::int64_t rowCount() const`.
template <typename Request, typename Sink>
class ProviderChain
{
public:
    explicit ProviderChain(std::string quantity) : m_quantity(std::move(quantity)) {}

    ProviderChain& add(std::unique_ptr<QuantityProvider<Request, Sink>> provider)
    {
        m_providers.push_back(std::move(provider));
        return *this;
    }
    [[nodiscard]] const std::string& quantity() const noexcept { return m_quantity; }
    [[nodiscard]] QuantityInfo info() const
    {
        QuantityInfo result;
        result.quantity = m_quantity;
        for (const auto& provider : m_providers)
            result.providers.push_back(ProviderInfo{provider->name(), provider->approximate()});
        return result;
    }

    // Runs the providers in order, each seeing what the ones before it gave. Returns what each did.
    [[nodiscard]] std::vector<ProviderAttempt> resolve(const ResultQueryContext& context, const Request& request,
                                                       Sink& sink) const
    {
        std::vector<ProviderAttempt> attempts;
        std::int16_t position = 0;
        for (const auto& provider : m_providers)
        {
            ProviderAttempt attempt;
            attempt.provider = provider->name();
            attempt.approximate = provider->approximate();
            if (ProviderSwitches::isDisabled(attempt.provider))
            {
                attempt.reason = "switched off";
            }
            else
            {
                context.exec().checkCancelled();
                sink.beginProvider(position, ProviderInfo{attempt.provider, attempt.approximate});
                const auto outcome = provider->provide(context, request, sink);
                attempt.rows = outcome.rows;
                attempt.reason = outcome.reason;
            }
            attempts.push_back(std::move(attempt));
            ++position;
        }
        return attempts;
    }

private:
    std::string m_quantity;
    std::vector<std::unique_ptr<QuantityProvider<Request, Sink>>> m_providers;
};

// Every quantity that has several ways of obtaining it, with its providers in order: what `quantity_providers()`
// reports and what the test of the provider order reads.
[[nodiscard]] const std::vector<QuantityInfo>& allQuantities();

} // namespace oor::query
