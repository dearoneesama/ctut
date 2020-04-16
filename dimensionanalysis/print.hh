#pragma once

#include <ostream>
#include "dim.hh"

namespace dim {
    template<unitpw_t s, unitpw_t m, unitpw_t kg, unitpw_t A,
        unitpw_t K, unitpw_t mol, unitpw_t cd, class T>
    std::ostream &operator<<(std::ostream &out, quantity<dimVec<s, m, kg, A, K, mol, cd>, T> &q) {
        out << q.value
        << " [s: " << s
        << ", m: " << m
        << ", kg: " << kg
        << ", A: " << A
        << ", K: " << K
        << ", mol: " << mol
        << ", cd: " << cd
        << ']';
        return out;
    }
}
