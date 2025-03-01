#include "individual.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace individual {
    individual_t operator&(const individual_t &a, const individual_t &b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("differently sized bitwise operands");
        }

        individual_t result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = a[i] & b[i];
        }
        return result;
    }

    individual_t operator|(const individual_t &a, const individual_t &b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("differently sized bitwise operands");
        }

        individual_t result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = a[i] | b[i];
        }
        return result;
    }

    individual_t operator^(const individual_t &a, const individual_t &b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("differently sized bitwise operands");
        }

        individual_t result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = a[i] ^ b[i];
        }
        return result;
    }

    individual_t operator~(const individual_t &a) {
        individual_t result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = ~a[i];
        }
        return result;
    }

    std::ostream &operator<<(std::ostream &os, const individual_t &x) {
        for (auto b : x) {
            os << (b ? '1' : '0');
        }
        return os;
    }

    std::string to_string(const individual_t &x) {
        size_t n = x.size();
        std::string result(n, '0');
        for (size_t i = 0; i < n; ++i) {
            result[i] = (x[i] == 0) ? '0' : '1';
        }
        return result;
    }

    size_t to_bits_be(const individual_t &x) {
        assert(x.size() <= (sizeof(size_t) << 3));
        size_t bits = 0;
        int n = x.size();
        for (int k = 0; k < n; ++k) {
            bits |= (x[k] & 1) * (1 << (n - 1 - k));
        }
        return bits;
    }

    size_t to_bits_le(const individual_t &x) {
        assert(x.size() <= (sizeof(size_t) << 3));
        size_t bits = 0;
        int n = x.size();
        for (int k = 0; k < n; ++k) {
            bits |= (x[k] & 1) * (1 << k);
        }
        return bits;
    }

    using objective::fn_t;
    using objective::val_t;
    using pareto::order;

    order compare(const individual_t &a, const individual_t &b, const fn_t &f) {
        val_t fa = f(a);
        val_t fb = f(b);
        return pareto::compare(fa, fb);
    }

    bool strictly_dominates(const individual_t &a, const individual_t &b, const fn_t &f) {
        val_t fa = f(a);
        val_t fb = f(b);
        return pareto::strictly_dominates(fa, fb);
    }

    bool dominates(const individual_t &a, const individual_t &b, const fn_t &f) {
        val_t fa = f(a);
        val_t fb = f(b);
        return pareto::dominates(fa, fb);
    }
}; // namespace individual

namespace objective {
    std::ostream &operator<<(std::ostream &os, const val_t &v) {
        size_t n = v.size();
        os << '[';
        if (n > 0) {
            os << v[0];
        }
        for (int i = 1; i < n; ++i) {
            os << ", " << v[i];
        }
        os << ']';
        return os;
    }
} // namespace objective

namespace pareto {
    order compare(const val_t &a, const val_t &b) {
        size_t n = a.size();
        assert(n == b.size());

        order out = order::equivalent;
        for (size_t i = 0; i < n; ++i) {
            double ai = a[i];
            double bi = b[i];
            if (ai == bi) {
                continue;
            }
            order cmp = ai <=> bi;
            if (out != order::equivalent && cmp != out) {
                return order::unordered;
            }
            out = cmp;
        }
        return out;
    }

    bool strictly_dominates(const val_t &a, const val_t &b) { return compare(a, b) > 0; }

    /* Returns `true` if the left objective value Pareto-dominates
        the right value. */
    bool dominates(const val_t &a, const val_t &b) { return compare(a, b) >= 0; }
}; // namespace pareto