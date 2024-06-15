//
// Created by Ricardo Evans on 2023/6/4.
//

#ifndef NS3_MPI_APPLICATION_OPERATION_H
#define NS3_MPI_APPLICATION_OPERATION_H

#include <coroutine>
#include <functional>
#include <optional>
#include <vector>
#include <utility>

#include "mpi-operation-awaitable.h"
#include "mpi-operation-trait.h"
#include "mpi-operation-type.h"

namespace ns3 {
    template<typename R>
    class MPIOperation {
    private:
        struct Promise {
            bool terminated = false;
            std::size_t reference = 0;
            std::optional<R> result;
            std::optional<std::exception_ptr> exception;
            std::vector<std::function<void(std::optional<R> &, std::optional<std::exception_ptr> &)>> continuations;

            constexpr std::suspend_never initial_suspend() const noexcept {
                return {};
            }

            constexpr ConditionalAwaitable final_suspend() const noexcept {
                return reference <= 0;
            }

            MPIOperation get_return_object() noexcept {
                return MPIOperation{Coroutine::from_promise(*this)};
            }

            template<AwaitableConcept T>
            constexpr T &&await_transform(T &&t) const noexcept {
                return std::forward<T>(t);
            }

            template<MPIOperationConcept T>
            constexpr auto await_transform(T &&operation) const noexcept {
                return MPIOperationAwaitable{std::forward<T>(operation)};
            }

            template<typename U=R>
            void terminate(U &&_result) {
                if (terminated) {
                    return;
                }
                terminated = true;
                result = std::forward<U>(_result);
            }

            template<typename ...Args>
            void terminate(std::in_place_t, Args &&...args) {
                if (terminated) {
                    return;
                }
                terminated = true;
                result.emplace(std::forward<Args>(args)...);
            }

            template<typename U=R>
            void return_value(U &&_result) {
                if (!terminated) {
                    result = std::forward<U>(_result);
                }
                terminated = true;
                complete();
            }

            void unhandled_exception() {
                if (!terminated) {
                    exception = std::current_exception();
                }
                terminated = true;
                complete();
            }

        private:
            void complete() {
                for (auto &f: continuations) {
                    f(result, exception);
                }
                continuations.clear();
            }
        };

        inline void releaseHandle() const {
            if (handle) {
                auto &promise = handle.promise();
                --promise.reference;
                if (promise.reference <= 0 && handle.done()) {
                    handle.destroy();
                }
            }
        }

        inline void checkHandle() const {
            if (!handle) {
                throw std::runtime_error("null operation");
            }
        }

    public:
        using promise_type = Promise;
        using Coroutine = std::coroutine_handle<promise_type>;

        MPIOperation() noexcept = default;

        explicit MPIOperation(const Coroutine &handle) noexcept: handle(handle) {
            if (handle) {
                ++this->handle.promise().reference;
            }
        }

        explicit MPIOperation(Coroutine &&handle) noexcept: handle(std::move(handle)) {
            if (handle) {
                ++this->handle.promise().reference;
            }
        }

        MPIOperation(MPIOperation &&operation) noexcept: handle(std::exchange(operation.handle, nullptr)) {}

        MPIOperation(const MPIOperation &operation) noexcept: MPIOperation(operation.handle) {}

        MPIOperation &operator=(const MPIOperation &operation) noexcept {
            releaseHandle();
            handle = operation.handle;
            if (handle) {
                ++handle.promise().reference;
            }
            return *this;
        }

        MPIOperation &operator=(MPIOperation &&operation) noexcept {
            releaseHandle();
            handle = std::exchange(operation.handle, nullptr);
            return *this;
        }

        constexpr inline Coroutine coroutine() const noexcept {
            return handle;
        }

        const R &result() const {
            checkHandle();
            auto &promise = handle.promise();
            if (promise.exception.has_value()) {
                std::rethrow_exception(std::exchange(promise.exception, std::nullopt).value());
            }
            return promise.result.value();
        }

        R &result() {
            checkHandle();
            auto &promise = handle.promise();
            if (promise.exception.has_value()) {
                std::rethrow_exception(std::exchange(promise.exception, std::nullopt).value());
            }
            return promise.result.value();
        }

        void onComplete(std::function<void()> &&function) const {
            checkHandle();
            onComplete([function = std::move(function)](auto &, auto &) { function(); });
        }

        void onComplete(const std::function<void()> &function) const {
            checkHandle();
            onComplete([function](auto &, auto &) { function(); });
        }

        void onComplete(std::function<void(std::optional<R> &, std::optional<std::exception_ptr> &)> &&function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function(promise.result, promise.exception);
            } else {
                promise.continuations.emplace_back(std::move(function));
            }
        }

        void onComplete(const std::function<void(std::optional<R> &, std::optional<std::exception_ptr> &)> &function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function(promise.result, promise.exception);
            } else {
                promise.continuations.emplace_back(function);
            }
        }

        bool resume() const {
            checkHandle();
            if (done()) {
                return true;
            }
            handle.resume();
            return done();
        }

        template<typename T=R>
        void terminate(T &&result) const {
            checkHandle();
            if (done()) {
                return;
            }
            handle.promise().terminate(std::forward<T>(result));
            while (!handle.done()) {
                handle.resume();
            }
        }

        template<typename ...Args>
        void terminate(std::in_place_t, Args &&...args) const {
            checkHandle();
            if (done()) {
                return;
            }
            handle.promise().terminate(std::in_place, std::forward<Args>(args)...);
            while (!handle.done()) {
                handle.resume();
            }
        }

        bool done() const {
            checkHandle();
            return handle.done() || handle.promise().terminated;
        }

        template<typename F>
        requires std::is_invocable_v<F, R &&>
        MPIOperation<std::invoke_result_t<F, R &&>> then(F function) const {
            checkHandle();
            co_return function(std::move(co_await *this));
        }

        virtual ~MPIOperation() noexcept {
            releaseHandle();
        }

    private:
        Coroutine handle;
    };

    template<>
    class MPIOperation<void> {
    private:
        struct Promise {
            bool terminated = false;
            std::size_t reference = 0;
            std::optional<std::exception_ptr> exception;
            std::vector<std::function<void(std::optional<std::exception_ptr> &)>> continuations;

            constexpr std::suspend_never initial_suspend() const noexcept {
                return {};
            }

            constexpr ConditionalAwaitable final_suspend() const noexcept {
                return reference <= 0;
            }

            MPIOperation get_return_object() noexcept {
                return MPIOperation{Coroutine::from_promise(*this)};
            }

            template<AwaitableConcept T>
            constexpr T &&await_transform(T &&t) const noexcept {
                return std::forward<T>(t);
            }

            template<MPIOperationConcept T>
            constexpr MPIOperationAwaitable<MPIOperationResultType<T>> await_transform(T &&operation) const noexcept {
                return MPIOperationAwaitable<MPIOperationResultType<T>>{std::forward<T>(operation)};
            }

            void terminate() {
                if (terminated) {
                    return;
                }
                terminated = true;
            }

            void return_void() {
                terminated = true;
                complete();
            }

            void unhandled_exception() {
                if (!terminated) {
                    exception = std::current_exception();
                }
                terminated = true;
                complete();
            }

        private:
            void complete() {
                for (auto &f: continuations) {
                    f(exception);
                }
                continuations.clear();
            }
        };

        constexpr inline void releaseHandle() const {
            if (handle) {
                auto &promise = handle.promise();
                --promise.reference;
                if (promise.reference <= 0 && handle.done()) {
                    handle.destroy();
                }
            }
        }

        constexpr inline void checkHandle() const {
            if (!handle) {
                throw std::runtime_error("null operation");
            }
        }

    public:
        using promise_type = Promise;
        using Coroutine = std::coroutine_handle<promise_type>;

        MPIOperation() noexcept = default;

        explicit MPIOperation(const Coroutine &handle) noexcept: handle(handle) {
            if (handle) {
                ++this->handle.promise().reference;
            }
        }

        explicit MPIOperation(Coroutine &&handle) noexcept: handle(std::move(handle)) {
            if (handle) {
                ++this->handle.promise().reference;
            }
        }

        MPIOperation(MPIOperation &&operation) noexcept: handle(std::exchange(operation.handle, nullptr)) {}

        MPIOperation(const MPIOperation &operation) noexcept: MPIOperation(operation.handle) {}

        MPIOperation &operator=(const MPIOperation &operation) noexcept {
            releaseHandle();
            handle = operation.handle;
            if (handle) {
                ++handle.promise().reference;
            }
            return *this;
        }

        MPIOperation &operator=(MPIOperation &&operation) noexcept {
            releaseHandle();
            handle = std::exchange(operation.handle, nullptr);
            return *this;
        }

        constexpr inline Coroutine coroutine() const noexcept {
            return handle;
        }

        void result() const {
            checkHandle();
            auto &promise = handle.promise();
            if (promise.exception.has_value()) {
                std::rethrow_exception(promise.exception.value());
            }
        }

        void onComplete(std::function<void()> &&function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function();
            } else {
                promise.continuations.emplace_back([function = std::move(function)](auto &) { function(); });
            }
        }

        void onComplete(const std::function<void()> &function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function();
            } else {
                promise.continuations.emplace_back([function](auto &) { function(); });
            }
        }

        void onComplete(std::function<void(std::optional<std::exception_ptr> &)> &&function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function(promise.exception);
            } else {
                promise.continuations.emplace_back(std::move(function));
            }
        }

        void onComplete(const std::function<void(std::optional<std::exception_ptr> &)> &function) const {
            checkHandle();
            auto &promise = handle.promise();
            if (done()) {
                function(promise.exception);
            } else {
                promise.continuations.emplace_back(function);
            }
        }

        bool resume() const {
            checkHandle();
            if (done()) {
                return true;
            }
            handle.resume();
            return done();
        }

        void terminate() const {
            checkHandle();
            if (done()) {
                return;
            }
            handle.promise().terminate();
            while (!handle.done()) {
                handle.resume();
            }
        }

        bool done() const {
            checkHandle();
            return handle.done() || handle.promise().terminated;
        }

        template<typename F>
        requires std::is_invocable_v<F, void>
        MPIOperation<std::invoke_result_t<F, void>> then(F function) const {
            checkHandle();
            co_return function(co_await *this);
        }

        virtual ~MPIOperation() noexcept {
            releaseHandle();
        }

    private:
        Coroutine handle;
    };

    template<typename T1, typename T2>
    constexpr std::strong_ordering operator<=>(const MPIOperation<T1> &o1, const MPIOperation<T2> &o2) noexcept {
        return o1.coroutine() <=> o2.coroutine();
    }

    template<typename T1, typename T2>
    constexpr bool operator==(const MPIOperation<T1> &o1, const MPIOperation<T2> &o2) noexcept {
        return o1.coroutine() == o2.coroutine();
    }

    template<typename R>
    requires std::is_default_constructible_v<R> || std::is_void_v<R>
    MPIOperation<R> makeMPIOperation() {
        co_await std::suspend_always{};
        if constexpr (std::is_void_v<R>) {
            co_return;
        } else {
            co_return R{};
        }
    }

    template<typename R>
    requires std::is_invocable_v<R>
    MPIOperation<std::invoke_result_t<R>> makeMPIOperation(R result) {
        co_await std::suspend_always{};
        co_return result();
    }

    template<typename C, typename R>
    requires std::is_invocable_r_v<bool, C> and std::is_invocable_v<R>
    MPIOperation<std::invoke_result_t<R>> makeMPIOperation(C condition, R result) {
        while (!condition()) {
            co_await std::suspend_always{};
        }
        co_return result();
    }
}

#endif //NS3_MPI_APPLICATION_OPERATION_H
