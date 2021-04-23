// requires -std=c++17 or above
#pragma once
#include <utility>
#include <tuple>

namespace oneesama {

    // Defer the execution of fn to the point when the destructor is run
    template<class Fn>
    struct defer {
        Fn fn;
        ~defer() { fn(); }
    };

    template<class Fn>
    defer(Fn &&) -> defer<Fn>;

    // The lambda wrapper can receive a lambda object and convert it into a c function pointer
    // and a ctx parameter typed sa void *. This is useful when users want to pass a closure into
    // a C interface that only accepts function pointers - which is not possible. By doing the conversion,
    // it is possible.
    template<class Fn>
    class l2cf;

    template<class Fn, class R, class ...FArgs>
    class l2cf<auto (Fn::*)(bool &, FArgs...) -> R> {
    public:
        // Construct a lambda wrapper using the passed lambda.
        // To create a function pointer with type (Foo, Bar, void *) -> R, please pass a functor with
        // type (bool &, Foo, Bar) -> R. Variables captures are fine. The first bool parameter is used to
        // determine whether to delete the heap object after the invocation of the created C function.
        // hence unless clean_up_on_destruct is true, assign this bool to true whenever you are done with it.
        template<class PassedFn, std::enable_if_t<std::is_convertible_v<PassedFn, Fn>> * = nullptr>
        constexpr l2cf(PassedFn &&p)
            : fn_{new fn_wrapper_t{std::forward<PassedFn>(p)}}
        {}

        constexpr l2cf(const l2cf &o)
            : l2cf{o.fn_->fn}
        {}

        constexpr l2cf(l2cf &&o) noexcept
            : fn_{o.fn_}, clean_up_on_destruct_{o.clean_up_on_destruct_}
        {
            o.fn_ = nullptr;
            o.clean_up_on_destruct_ = false;
        }

        constexpr auto &operator=(l2cf o) noexcept {
            std::swap(fn_, o.fn_);
            std::swap(clean_up_on_destruct_, o.clean_up_on_destruct_);
            return *this;
        };

        ~l2cf() noexcept {
            /* no checking performed */
            if (clean_up_on_destruct_)
                delete fn_;
        }

        // If this is true, the heap objects allocated by this wrapper will be destructed when the wrapper
        // object is destructed. Default to true.
        constexpr auto &clean_up_on_destruct(bool v) noexcept {
            clean_up_on_destruct_ = v;
            return *this;
        }

        // Returns the ctx pointer used by the C function.
        constexpr void *get_ctx() noexcept {
            return fn_;
        }

        // Returns a stateless lambda object that is convertible to a C function pointer.
        auto constexpr get_cfnptr() const noexcept {
            // an awkward declaration:
            // constexpr R (*get_cfnptr() const noexcept)(FArgs ..., void *);
            return [] (FArgs ...args, void *ctx) {
                auto *fw = static_cast<fn_wrapper_t *>(ctx);
                auto clean_up = false;
                auto unused = defer{[fw, &clean_up] {
                    if (clean_up)
                        delete fw;
                }};
                return fw->fn(clean_up, args...);
            };
        }

    private:
        struct fn_wrapper_t { Fn fn; } *fn_;
        bool clean_up_on_destruct_ = true;
    };

    template<class Fn, class R, class ...Args>
    class l2cf<auto (Fn::*)(Args...) const -> R> : public l2cf<auto (Fn::*)(Args...) -> R>
    {};

    template<class Fn>
    l2cf(Fn &&) -> l2cf<decltype(&Fn::operator())>;

    // Gets fp, ctx, l2cf at the same time.
    template<class Fn>
    auto constexpr make_quick_cf_pair(Fn &&f) {
        auto wrapper = l2cf{std::forward<Fn>(f)};
        return std::make_tuple(wrapper.get_cfnptr(), wrapper.get_ctx(), std::move(wrapper));
    }
}
