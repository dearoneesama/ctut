#include <cassert>
#include <iostream>
#include "dim.hh"
#include "literal.hh"
#include "print.hh"

using namespace dim;

int main(void) {
    // can define a sample unit -> m/s^2 = ((m/s)/s)
    using gravity = dimDivision_t<metre, second, second>;
    // N = kg * m/s^2
    using newton = dimMultiply_t<kilogram, gravity>;
    // V = kg*m^2/(A*s^3)
    using volt = dimDivision_t<
        dimMultiply_t<kilogram, metre, metre>,
        dimMultiply_t<ampere, second, second, second>
    >;

    // can also compute values and automatically get correct units
    // define m = 4.5kg
    auto m = quantity<kilogram>{ 4.5 };
    // define v = 8.8m/2s = 4.4m/s
    auto v = quantity<metre>{ 8.8 } / quantity<second>{ 2 };
    // define t = 3s
    auto t = quantity<second>{ 3 };
    // define F = mv/t
    auto F = m * (v/t);

    // the value is...
    assert(F.value >= 6.599 && F.value <= 6.601);
    // we can make sure the unit is correct
    static_assert(std::is_same<decltype(F)::unit, newton>::value);
    // print its value followed by units
    std::cout << F << '\n';

    // construct a value with literals...
    auto somevolts = 3.3_kg*2.0_m*1.0_m / (0.3_A*0.2_s*1.0_s*1.0_s);
    // make sure its type is right...
    static_assert(std::is_same<decltype(somevolts)::unit, volt>::value);
    std::cout << somevolts << '\n';
    // can add another value of same type, and multiply by a scalar
    auto somemorevolts = -2.0_sc * (somevolts + quantity<volt>{ 2.2 });
    // type is unchanged
    static_assert(std::is_same<decltype(somemorevolts)::unit, volt>::value);
    std::cout << somemorevolts << '\n';
    // this is not allowed!
    // auto wrong = somevolts + F;
}
