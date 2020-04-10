#pragma once
#include "dim.hh"

namespace dim {
    auto operator""_sc(long double x) { 
        return quantity<scalar>{ x };
    }
    auto operator""_s(long double x) { 
        return quantity<second>{ x };
    }
    auto operator""_m(long double x) { 
        return quantity<metre>{ x };
    }
    auto operator""_kg(long double x) { 
        return quantity<kilogram>{ x };
    }
    auto operator""_A(long double x) { 
        return quantity<ampere>{ x };
    }
    auto operator""_K(long double x) { 
        return quantity<kelvin>{ x };
    }
    auto operator""_mol(long double x) { 
        return quantity<mole>{ x };
    }
    auto operator""_cd(long double x) { 
        return quantity<candela>{ x };
    }
}
