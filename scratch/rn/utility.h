#ifndef NS3_UTILITY_H
#define NS3_UTILITY_H

#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>

template<typename T>
concept Container=requires(const T &t){
    { t.Get(std::declval<std::size_t>()) };
    { t.Begin() };
    { t.End() };
};

template<std::size_t N, Container T>
requires (N == 1)
auto ContainerPattern(T &&container, std::size_t offset = 0) {
    return std::make_tuple(container.Get(offset));
}

template<std::size_t N, Container T>
requires (N == 0)
auto ContainerPattern(T &&container, std::size_t offset = 0) {
    return std::tuple<>{};
}

template<std::size_t N, Container T>
requires (N > 1)
auto ContainerPattern(T &&container, std::size_t offset = 0) {
    auto tuple = std::make_tuple(container.Get(offset));
    return std::tuple_cat(tuple, ContainerPattern<N - 1>(std::forward<T>(container), offset + 1));
}

template<std::size_t N, typename T, typename F>
requires std::is_invocable_v<F, const T &, std::size_t> && (N == 1)
auto ContainerPattern(const T &container, F &&mapper, std::size_t offset = 0) {
    return std::make_tuple(mapper(container, offset));
}

template<std::size_t N, typename T, typename F>
requires std::is_invocable_v<F, const T &, std::size_t> && (N == 0)
auto ContainerPattern(const T &container, F &&mapper, std::size_t offset = 0) {
    return std::tuple<>{};
}

template<std::size_t N, typename T, typename F>
requires std::is_invocable_v<F, const T &, std::size_t> && (N > 1)
auto ContainerPattern(const T &container, F &&mapper, std::size_t offset = 0) {
    auto tuple = std::make_tuple(mapper(container, offset));
    return std::tuple_cat(tuple, ContainerPattern<N - 1>(container, std::forward<F>(mapper), offset + 1));
}

#endif //NS3_UTILITY_H
