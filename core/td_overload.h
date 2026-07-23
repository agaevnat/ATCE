#pragma once

// Standard visitor helper recommended by TDLib for td_api::downcast_call:
// overloaded(lambda_for_type_a, lambda_for_type_b, ...) builds a single
// callable with one operator() overload per lambda.

namespace exporter::detail {

template <class... Fs>
struct overload;

template <class F>
struct overload<F> : public F {
    explicit overload(F f) : F(f) {
    }
};

template <class F, class... Fs>
struct overload<F, Fs...> : public overload<F>, public overload<Fs...> {
    overload(F f, Fs... fs) : overload<F>(f), overload<Fs...>(fs...) {
    }
    using overload<F>::operator();
    using overload<Fs...>::operator();
};

}  // namespace exporter::detail

namespace exporter {

template <class... F>
auto overloaded(F... f) {
    return detail::overload<F...>(f...);
}

}  // namespace exporter
