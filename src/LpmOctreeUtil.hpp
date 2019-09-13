#ifndef LPM_OCTREE_UTIL_HPP
#define LPM_OCTREE_UTIL_HPP

#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "LpmUtilities.hpp"
#include "LpmKokkosUtil.hpp"
#include "LpmBox3d.hpp"
#include "Kokkos_Core.hpp"
#include <cstdint>
#include <iostream>

namespace Lpm {
namespace Octree {

typedef uint_fast32_t key_type;
typedef uint32_t id_type;
typedef uint_fast64_t code_type;

/// 2 raised to a nonnegative integer power
template <typename T, typename IntT2=int> KOKKOS_INLINE_FUNCTION
T pintpow2(const IntT2& k) {
    T result = 1;
    for (int i=1; i<=k; ++i) {
        result *= 2;
    }
    return result;
}

/**
    key = x1y1z1x2y2z2...xdydzd
    
    where bits xi, yi, zi, correspond to left (0) and right(1) of the midpoint of the octree
    node i's parent in the x,y,z direction
*/
template <typename CPtType> KOKKOS_INLINE_FUNCTION
key_type compute_key(const CPtType& pos, const int& level_depth) {
    /// assume root box is [-1,1]^3
    Real cx, cy, cz; // crds of box centroid
    Real half_len = 1.0; // half-length of box edges
    cx = 0;
    cy = 0;
    cz = 0;
    key_type key = 0;
    for (int i=0; i<level_depth; ++i) {
        const int pl = 3*(level_depth-1);
        const bool xr = (pos(0) > cx);
        const bool yr = (pos(1) > cy);
        const bool zr = (pos(2) > cz);
        if (xr) {
            key += pintpow2<key_type>(pl-1);
            cx += half_len;
        }
        else {
            cx -= half_len;
        }
        if (yr) {
            key += pintpow2<key_type>(pl-2);
            cy += half_len;
        }
        else {
            cy -= half_len;
        }
        if (zr) {
            key += pintpow2<key_type>(pl-3);
            cz += half_len;
        }
        else {
            cz -= half_len;
        }
        half_len *= 0.5;
    }
    return key;
}

KOKKOS_INLINE_FUNCTION
code_type encode(const key_type key, const id_type id) {
    return ((key<<32) + id);
}

KOKKOS_INLINE_FUNCTION
id_type decode_id(const code_type& code) {
    return id_type(code);
}

struct PermuteKernel {
    ko::View<Real*[3]> outpts;
    ko::View<Real*[3]> inpts;
    ko::View<uint_fast64_t*> codes;
    
    PermuteKernel(ko::View<Real*[3]>& op, const ko::View<Real*[3]>& ip, const ko::View<uint_fast64_t*>& c) :
        outpts(op), inpts(ip), codes(c) {}
        
    KOKKOS_INLINE_FUNCTION
    void operator() (const id_type& i) const {
        const id_type old_id = decode_id(codes(i));
        for (int j=0; j<3; ++j) {
            outpts(i,j) = inpts(old_id,j);
        }
    }
};

struct MarkDuplicates {
    ko::View<Index*> flags;
    ko::View<code_type*> codes;
    
    MarkDuplicates(ko::View<Index*> f, const ko::View<code_type*>& c) : flags(f), codes(c) {}
    
    struct MarkTag {};
    struct ScanTag {};
    struct CountTag {};
    
    KOKKOS_INLINE_FUNCTION
    void operator () (const MarkTag&, const Index& i) const {
        if (i > 0) {
            flags(i) = ((codes(i) != codes(i-1)) ? 1 : 0);
        }
        else {
            flags(i) = 1;
        }
    }
    
    KOKKOS_INLINE_FUNCTION
    void operator() (const ScanTag&, const Index& i, Index& ct, const bool& final_pass) const {
        const Index old_val = flags(i);
        ct += old_val;
        if (final_pass) {
            flags(i) += old_val;
        }
    }
};

struct CopyIfKernel {
    ko::View<Index*> flags;
    ko::View<code_type*> codes_in;
    ko::View<code_type*> codes_out;
    
    CopyIfKernel(ko::View<code_type*>& oc, const ko::View<Index*>& f, const ko::View<code_type*>& ic) : 
        flags(f), codes_in(ic), codes_out(oc) {}
    
    KOKKOS_INLINE_FUNCTION
    void operator () (const Index& i) const {
        bool newval = true;
        if (i > 0) newval = (flags(i) > flags(i-1));
        if (newval) {
            codes_out(flags(i)-1) = codes_in(i);
        }
    }
};

}
}
#endif
