#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cassert>
#include <cstdint>
#include <utility>

template <typename T>
class bit_transition_verifier
{
public:
    bit_transition_verifier(T mask, T initial_value = 0) noexcept:
        _mask(mask),
        _last_value(initial_value & mask)
    {}

    bool operator () (T value) noexcept
    {
        auto masked_value = value & _mask;
        bool found_changes = (_last_value ^ masked_value) != 0;
        _last_value = masked_value;
        return found_changes;
    }

protected:
    T _mask;
    T _last_value;
};


template <typename Result, typename... Args>
struct function_utility
{
    using function = std::function<Result(Args...)>;
    using function_ptr = Result (*)(Args...);

    static size_t internal_ptr(const function& item) noexcept
    {
        return reinterpret_cast<size_t>(item.template target<function_ptr>());
    }

    // for hashing
    size_t operator()(const function& item) const noexcept
    {
        return internal_ptr(item);
    }

    // for comparison
    bool operator()(const function& a, const function& b) const noexcept
    {
        return internal_ptr(a) < internal_ptr(b);
    }
};

template <typename T>
class bit_transition_verifier_set
{
public:
    using notifier = std::function<void(T value)>;

    void add(T mask, notifier n)
    {
        std::uint8_t bit_no = 0;
        for( ; mask != 0; mask >>= 1, ++bit_no)
        {
            if ((mask & 1) != 0)
            {
                _map[bit_no].notifiers.insert(n);
            }
        }
    }

    void operator () (T value)
    {
        notifier_set notifiers;
        constexpr std::uint8_t bit_count = sizeof(T) << 3;
        for(std::uint8_t bit_no = 0; bit_no != bit_count; value >>= 1, ++bit_no)
        {
            bool new_value = (value & 1) != 0;
            auto& data = _map[bit_no];
            if (data.last_value != new_value)
            {
                notifiers.insert(data.notifiers.begin(), data.notifiers.end());
                data.last_value = new_value;
            }
        }

        // call notifiers
        for(auto& item : notifiers)
        {
            try
            {
                item(value);
            }
            catch(...)
            {

            }
        }
    }

protected:
    using notifier_set = std::unordered_set<notifier, function_utility<void, T>, function_utility<void, T>>;

    struct bit_data
    {
        notifier_set notifiers;
        bool last_value = false;

        bit_data() = default;
    };

    using bit_no_notifiers_map = std::unordered_map<std::uint8_t, bit_data>;

    bit_no_notifiers_map _map;
};


int main()
{
    bit_transition_verifier<std::uint8_t> tv(0b00000011);
    bool v1 = tv(0b00000000);
    assert(!v1);
    bool v2 = tv(0b00000010);
    assert(v2);
    bool v3 = tv(0b00000010);
    assert(!v3);
    bool v4 = tv(0b00000000);
    assert(v4);
    bool v5 = tv(0b00000001);
    assert(v5);

    bool vs1 = false;
    bool vs2 = false;

    bit_transition_verifier_set<std::uint8_t> tvs;
    tvs.add(0b10000001, [&](std::uint8_t){ vs1 = true; });
    tvs.add(0b00000011, [&](std::uint8_t){ vs2 = true; });

    tvs(0b00000000);
    tvs(0b10000000);
    tvs(0b00000000);
    tvs(0b00000001);

    assert(vs1);
    assert(vs2);

    return 0;
}
