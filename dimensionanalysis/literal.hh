#pragma once
#include "dim.hh"

namespace dim {
    auto constexpr operator""_sc(long double x) { 
        return quantity<scalar>{ x };
    }
    auto constexpr operator""_s(long double x) { 
        return quantity<second>{ x };
    }
    auto constexpr operator""_m(long double x) { 
        return quantity<metre>{ x };
    }
    auto constexpr operator""_kg(long double x) { 
        return quantity<kilogram>{ x };
    }
    auto constexpr operator""_A(long double x) { 
        return quantity<ampere>{ x };
    }
    auto constexpr operator""_K(long double x) { 
        return quantity<kelvin>{ x };
    }
    auto constexpr operator""_mol(long double x) { 
        return quantity<mole>{ x };
    }
    auto constexpr operator""_cd(long double x) { 
        return quantity<candela>{ x };
    }
}
