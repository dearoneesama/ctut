#pragma once

namespace dim {

    using unitpw_t = int;

    namespace fctr {
        template<unitpw_t lhs, unitpw_t rhs>
        struct bin_add {
            using type = bin_add;
            static constexpr unitpw_t value = lhs + rhs;
        };

        template<unitpw_t lhs, unitpw_t rhs>
        struct bin_sub {
            using type = bin_sub;
            static constexpr unitpw_t value = lhs - rhs;
        };
    }

    template<unitpw_t, unitpw_t, unitpw_t, unitpw_t, unitpw_t, unitpw_t, unitpw_t>
    struct dimVec {
        using type = dimVec;
    };

    using scalar   = dimVec<0, 0, 0, 0, 0, 0, 0>;
    using second   = dimVec<1, 0, 0, 0, 0, 0, 0>;
    using metre    = dimVec<0, 1, 0, 0, 0, 0, 0>;
    using kilogram = dimVec<0, 0, 1, 0, 0, 0, 0>;
    using ampere   = dimVec<0, 0, 0, 1, 0, 0, 0>;
    using kelvin   = dimVec<0, 0, 0, 0, 1, 0, 0>;
    using mole     = dimVec<0, 0, 0, 0, 0, 1, 0>;
    using candela  = dimVec<0, 0, 0, 0, 0, 0, 1>;

    // manipulate dimension arrays using binary functor
    template<template<unitpw_t, unitpw_t> class F, class ...Ds>
    struct dimTransform;
    template<
        template<unitpw_t, unitpw_t> class F,
        template<unitpw_t...> class D1, unitpw_t ...d1,
        template<unitpw_t...> class D2, unitpw_t ...d2
    >
    struct dimTransform<F, D1<d1...>, D2<d2...>> {
        using type = dimVec<F<d1, d2>::value...>;
    };
    template<template<unitpw_t, unitpw_t> class F, class D1, class D2, class ...Dn>
    struct dimTransform<F, D1, D2, Dn...>
        : dimTransform<F, typename dimTransform<F, D1, D2>::type, Dn...> {};
    // add dimension arrays, effectively multiplying units
    template<class ...Dn>
    using dimMultiply_t = typename dimTransform<fctr::bin_add, Dn...>::type;
    // subtract dimension arrays, effectively dividing units
    // warning: it is a left fold... meaning it will evaluate from left to right
    template<class ...Dn>
    using dimDivision_t = typename dimTransform<fctr::bin_sub, Dn...>::type;

    template<class Dim, class T = long double>
    struct quantity {
        using unit = Dim;
        const T value;
        explicit constexpr quantity(const T x): value{x} {};
    };

    // negation of quantity
    template<class Dim, class T>
    auto constexpr operator-(const quantity<Dim, T> &me) {
        return quantity<Dim, T>{ -me.value };
    }
    // addition of quantities to produce same unit same dimension quantities
    template<class Dim, class T>
    auto constexpr operator+(const quantity<Dim, T> &lhs, const quantity<Dim, T> &rhs) {
        return quantity<Dim, T>{ lhs.value + rhs.value };
    }
    // subtraction of quantities to produce same unit same dimension quantities
    template<class Dim, class T>
    auto constexpr operator-(const quantity<Dim, T> &lhs, const quantity<Dim, T> &rhs) {
        return quantity<Dim, T>{ lhs.value - rhs.value };
    }
    // multiplication of quantities
    template<class D1, class D2, class T>
    auto constexpr operator*(const quantity<D1, T> &lhs, const quantity<D2, T> &rhs) {
        return quantity<dimMultiply_t<D1, D2>, T>{ lhs.value * rhs.value };
    }
    // division of quantities
    template<class D1, class D2, class T>
    auto constexpr operator/(const quantity<D1, T> &lhs, const quantity<D2, T> &rhs) {
        return quantity<dimDivision_t<D1, D2>, T>{ lhs.value / rhs.value };
    }
}
