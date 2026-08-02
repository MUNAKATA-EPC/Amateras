#include <iostream>
#include <unordered_set>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <numeric>

template <typename T>
class set
{
private:
    std::unordered_set<T> elems;
    std::optional<std::unordered_set<T>> universal;

public:
    set() = default;

    explicit set(int capacity)
    {
        elems.reserve(capacity);
    }

    set(int capacity, const set &U)
    {
        elems.reserve(capacity);
        universal = U.elems;
    }

    // 要素追加
    void add(const T &val) { elems.insert(val); }

    // ∈ 所属判定
    bool contains(const T &val) const
    {
        return elems.count(val) > 0;
    }

    // ⊆/⊇ 部分集合判定
    bool operator<=(const set &other) const
    {
        for (const auto &e : elems)
            if (!other.elems.count(e))
                return false;
        return true;
    }
    bool operator>=(const set &other) const { return other <= *this; }

    // ⊂/⊃ 真部分集合判定
    bool operator<(const set &other) const
    {
        return (*this <= other) && (elems.size() < other.elems.size());
    }
    bool operator>(const set &other) const { return other < *this; }

    // = 等価判定
    bool operator==(const set &other) const { return elems == other.elems; }

    // ∩ 共通部分
    set operator&(const set &other) const
    {
        set result;
        for (const auto &e : elems)
            if (other.elems.count(e))
                result.add(e);
        return result;
    }

    // ∪ 和集合
    set operator|(const set &other) const
    {
        set result;
        result.elems = elems;
        result.elems.insert(other.elems.begin(), other.elems.end());
        return result;
    }

    // ∁ 補集合
    set operator~() const
    {
        if (!universal)
            return set();
        set result;
        for (const auto &e : *universal)
            if (!elems.count(e))
                result.add(e);
        return result;
    }

    // A \ B 差集合
    set operator-(const set &other) const
    {
        set result;
        for (const auto &e : elems)
            if (!other.elems.count(e))
                result.add(e);
        return result;
    }

    // |A| 要素数
    size_t size() const { return elems.size(); }

    // 総和 Σx
    T sum() const
    {
        return std::accumulate(elems.begin(), elems.end(), T{});
    }

    // 平均 Σx / |A|
    double mean() const
    {
        if (elems.empty())
            throw std::logic_error("空集合です");
        return static_cast<double>(sum()) / elems.size();
    }

    // 最大値
    T max() const
    {
        if (elems.empty())
            throw std::logic_error("空集合です");
        return *std::max_element(elems.begin(), elems.end());
    }

    // 最小値
    T min() const
    {
        if (elems.empty())
            throw std::logic_error("空集合です");
        return *std::min_element(elems.begin(), elems.end());
    }

    // イテレータ
    /*このように使う
    for (const auto& e : A) {
    if (e % 2 == 0) std::cout << e << "\n";
    }*/
    auto begin() const { return elems.begin(); }
    auto end() const { return elems.end(); }
};