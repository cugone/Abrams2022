#pragma once

#include <algorithm>
#include <vector>

template<typename... ARGS>
class Event {
public:
    struct event_sub_t;
    using cb_t = void (*)(event_sub_t*, ARGS...);
    using cb_with_arg_t = void (*)(void*, ARGS...);

    struct event_sub_t {
        cb_t cb;
        void* secondary_cb;
        void* user_arg;
    };

    Event() = default;
    ~Event() = default;

    void Subscribe(void* user_arg, cb_with_arg_t cb) noexcept {
        event_sub_t sub;
        sub.cb = FunctionWithArgumentCallback;
        sub.secondary_cb = cb;
        sub.user_arg = user_arg;
        subscriptions.push_back(sub);
    }

    void Unsubscribe(void* user_arg, void* cb) noexcept {
        subscriptions.erase(std::remove_if(std::begin(subscriptions),
                                           std::end(subscriptions),
                                           [&cb, &user_arg](const event_sub_t& sub) {
                                               return (sub.secondary_cb == cb) && (sub.user_arg == user_arg);
                                           }),
                            std::end(subscriptions));
    }

    void Unsubscribe_by_argument(void* user_arg) noexcept {
        subscriptions.erase(std::remove_if(std::begin(subscriptions),
                                           std::end(subscriptions),
                                           [&user_arg](const event_sub_t& sub) {
                                               return sub.user_arg == user_arg;
                                           }),
                            std::end(subscriptions));
    }

    template<typename T>
    void Subscribe_method(T* obj, void (T::*mcb)(ARGS...)) noexcept {
        event_sub_t sub;
        sub.cb = MethodCallback<T, decltype(mcb)>;
        sub.secondary_cb = *(void**)(&mcb);
        sub.user_arg = obj;
        subscriptions.push_back(sub);
    }

    template<typename T>
    void Unsubscribe_method(T* obj, void (T::*mcb)(ARGS...)) noexcept {
        Unsubscribe(obj, *(void**)&mcb);
    }

    template<typename T>
    void Unsubscribe_object(T* obj) noexcept {
        Unsubscribe_by_argument(obj);
    }

    void Trigger(ARGS... args) const noexcept {
        for(const auto& sub : subscriptions) {
            sub.cb(&sub, args...);
        }
    }

    void Trigger(ARGS... args) noexcept {
        for(auto& sub : subscriptions) {
            sub.cb(&sub, args...);
        }
    }

private:
    std::vector<event_sub_t> subscriptions;

    static void FunctionWithArgumentCallback(event_sub_t* sub, ARGS... args) noexcept;

    template<typename T, typename MCB>
    static void MethodCallback(event_sub_t* sub, ARGS... args) noexcept;
};

template<typename... ARGS>
void Event<ARGS...>::FunctionWithArgumentCallback(event_sub_t* sub, ARGS... args) noexcept {
    cb_with_arg_t cb = (cb_with_arg_t)(sub->secondary_cb);
    cb(sub->user_arg, args...);
}

template<typename... ARGS>
template<typename T, typename MCB>
void Event<ARGS...>::MethodCallback(event_sub_t* sub, ARGS... args) noexcept {
    MCB mcb = *(MCB*)&(sub->secondary_cb);
    T* obj = (T*)(sub->user_arg);
    (obj->*mcb)(args...);
}
