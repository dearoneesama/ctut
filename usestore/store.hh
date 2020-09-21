#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <stdexcept>

namespace myrdx {

    template<class StateT, class ActionT>
    class store {
        static_assert(!std::is_reference<StateT>::value && std::is_move_assignable<StateT>::value);
    public:
        using state_t = StateT;
        using action_t = ActionT;
        // given previous state and action, returns next state, which triggers a state change
        using reducer_t = std::function<auto (const state_t &, const action_t &)->state_t>;
        // callbacks to be called after a state change
        using handler_t = std::function<auto ()->void>;

        template<class SttT>
        store(reducer_t reducer, SttT &&init_state)
            : reducer_{reducer ? std::move(reducer) : throw bad_std_func()}
            , state_{std::forward<SttT>(init_state)}
        {}

        // register a callback to be invoked any time an action has been dispatched
        // returns a function that unsubscribes the change listener
        auto subscribe(handler_t handler) {
            if (!handler) throw bad_std_func();

            auto target = ++curr_id;
            handlers_.emplace_back(handler_tagged { std::move(handler), target });

            return [target, &handlers_ = handlers_] {
                auto it = std::find_if(handlers_.begin(), handlers_.end(),
                    [&target](auto &&curr) {
                        return curr.id == target;
                    }
                );
                if (it != handlers_.end()) handlers_.erase(it);
            };
        }

        // dispatches an action to trigger a state change
        auto dispatch(const action_t &action) {
            state_ = reducer_(state_, action);
            for (auto &&f : handlers_) f.content();
        }

        // replaces the reducer currently used by the store to calculate the state
        auto replace_reducer(reducer_t new_reducer) {
            if (!new_reducer) throw bad_std_func();
            reducer_ = std::move(new_reducer);
        }

        // returns the current state. it is equal to the last value returned by the store's reducer
        const auto &get_state() const {
            return state_;
        }

    private:
        // each callback is associated with an id
        struct handler_tagged {
            handler_t content;
            long long id;
        };

        long long curr_id {0};

        reducer_t reducer_; // ensure nonempty
        std::vector<handler_tagged> handlers_; // ensure all std::func values are nonempty
        state_t state_;

        static auto bad_std_func() {
            return std::runtime_error{"requires a valid std::function object"};
        }
    };
}
