#ifndef NS3_MPI_APPLICATION_OPERATION_AWAITABLE_H
#define NS3_MPI_APPLICATION_OPERATION_AWAITABLE_H

#include <functional>
#include <utility>

#include "mpi-operation-trait.h"
#include "mpi-operation-type.h"

namespace ns3 {
    class ConditionalAwaitable {
    private:
        const bool condition;
    public:
        constexpr ConditionalAwaitable(bool condition) noexcept: condition(condition) {}

        constexpr bool await_ready() const noexcept {
            return condition;
        }

        constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}

        constexpr void await_resume() const noexcept {}

        ~ConditionalAwaitable() = default;
    };

    template<typename R>
    class MPIOperationAwaitable {
    public:
        constexpr explicit MPIOperationAwaitable(MPIOperation<R> &&operation) noexcept(std::is_nothrow_move_constructible_v<MPIOperation<R>>): operation(std::move(operation)) {}

        constexpr explicit MPIOperationAwaitable(const MPIOperation<R> &operation) noexcept(std::is_nothrow_copy_constructible_v<MPIOperation<R>>): operation(operation) {}

        constexpr MPIOperationAwaitable(MPIOperationAwaitable &&awaitable) noexcept(std::is_nothrow_move_constructible_v<MPIOperation<R>>) = default;

        MPIOperationAwaitable(const MPIOperationAwaitable &awaitable) noexcept(std::is_nothrow_copy_constructible_v<MPIOperation<R>>) = default;

        constexpr MPIOperationAwaitable &operator=(MPIOperationAwaitable &&awaitable) noexcept(std::is_nothrow_move_assignable_v<MPIOperation<R>>) = default;

        MPIOperationAwaitable &operator=(const MPIOperationAwaitable &awaitable) noexcept(std::is_nothrow_copy_assignable_v<MPIOperation<R>>) = default;

        constexpr bool await_ready() const noexcept {
            return operation.done();
        }

        void await_suspend(std::coroutine_handle<> h) {
            if constexpr (std::is_void_v<R>) {
                operation.onComplete([h](auto &) { h.resume(); });
            } else {
                operation.onComplete([h](auto &, auto &) { h.resume(); });
            }
        }

        decltype(auto) await_resume() noexcept(false) {
            return operation.result();
        }

        decltype(auto) await_resume() const noexcept(false) {
            return operation.result();
        }

        ~MPIOperationAwaitable() = default;

    private:
        MPIOperation<R> operation;
    };
}

#endif //NS3_MPI_APPLICATION_OPERATION_AWAITABLE_H
