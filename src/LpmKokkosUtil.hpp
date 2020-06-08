#ifndef LPM_KOKKOS_UTIL
#define LPM_KOKKOS_UTIL

#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "Kokkos_Core.hpp"
#include "Kokkos_Array.hpp"
#include <limits>
#include <cfloat>
/**
Kokkos-related utilities
*/
namespace Kokkos {

/** \brief Tuple type for || reduction.

    T is a plain old data type
    ndim is the number of T's in the tuple.

    Basic functions handled by superclass, Kokkos::Array
    This subclass adds the required operators for sum and product reductions.
*/
template <typename T, int ndim> struct Tuple : public Array<T,ndim> {
    KOKKOS_FORCEINLINE_FUNCTION
    Tuple() : Array<T,ndim>() {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] = 0;
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple(const T& val) : Array<T,ndim>() {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] = val;
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple(const T& v0, const T& v1) : Array<T,ndim>() {
        this->m_internal_implementation_private_member_data[0] = v0;
        this->m_internal_implementation_private_member_data[1] = v1;
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple(const T& v0, const T& v1, const T& v2) : Array<T,ndim>() {
        this->m_internal_implementation_private_member_data[0] = v0;
        this->m_internal_implementation_private_member_data[1] = v1;
        this->m_internal_implementation_private_member_data[2] = v2;
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple(const T* ptr) : Array<T,ndim>() {
        for (int i=0; i<ndim; ++i) {
            this->m_internal_implementation_private_member_data[i] = ptr[i];
        }
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple(const View<T*>& v) : Array<T,ndim>() {
        for (int i=0; i<ndim; ++i) {
            this->m_internal_implementation_private_member_data[i] = v[i];
        }
    }

    KOKKOS_INLINE_FUNCTION
    volatile T& operator[] (const int& i) volatile {return this->m_internal_implementation_private_member_data[i];}

    KOKKOS_INLINE_FUNCTION
    volatile typename std::add_const<T>::type & operator[] (const int& i) const volatile {
        return this->m_internal_implementation_private_member_data[i];
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple operator += (const Tuple<T,ndim>& o) {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] += o[i];
        return *this;
    }
    KOKKOS_INLINE_FUNCTION
    Tuple operator += (const volatile Tuple<T,ndim>& o) volatile {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] += o[i];
        return *this;
    }

    KOKKOS_FORCEINLINE_FUNCTION
    Tuple operator *= (const Tuple<T,ndim>& o) {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] *= o[i];
        return *this;
    }

    KOKKOS_INLINE_FUNCTION
    Tuple operator *= (const volatile Tuple<T,ndim>& o) volatile {
        for (int i=0; i<ndim; ++i)
            this->m_internal_implementation_private_member_data[i] *= o[i];
        return *this;
    }
};

template <>
struct reduction_identity<Tuple<Lpm::Real,3>> {
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,3> sum() {return Tuple<Lpm::Real,3>();}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,3> prod() {return Tuple<Lpm::Real,3>(1);}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,3> max() {return Tuple<Lpm::Real,3>(-DBL_MAX);}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,3> min() {return Tuple<Lpm::Real,3>(DBL_MAX);}
};

template <>
struct reduction_identity<Tuple<Lpm::Real,2>> {
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,2> sum() {return Tuple<Lpm::Real,2>();}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,2> prod() {return Tuple<Lpm::Real,2>(1);}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,2> max() {return Tuple<Lpm::Real,2>(-DBL_MAX);}
    KOKKOS_FORCEINLINE_FUNCTION static Tuple<Lpm::Real,2> min() {return Tuple<Lpm::Real,2>(DBL_MIN);}
};

}

namespace Lpm {
/**
    ExeSpaceUtils is a TeamPolicy factory.  Defines thread layout : number of teams, threads per team.

    CPU: 1 thread per team.
*/
template <typename ExeSpace=ko::DefaultExecutionSpace>
struct ExeSpaceUtils {
    using TeamPolicy = ko::TeamPolicy<ExeSpace>;

    static TeamPolicy get_default_team_policy(Int ni, Int nk) {
#ifdef MIMIC_GPU
        const int max_threads = ExeSpace::concurrency();
        const int team_size = max_threads < 7 ? max_threads : 7;
        return TeamPolicy(ni, team_size);
#else
        return TeamPolicy(ni, 1);
#endif
    }
};

/**
    Specialized policy for Cuda.

    GPU: <= 128 threads per team.
*/
#ifdef LPM_HAVE_CUDA
template <>
struct ExeSpaceUtils<ko::Cuda> {
    using TeamPolicy = ko::TeamPolicy<ko::Cuda>;

    static TeamPolicy get_default_team_policy(Int ni, Int nk) {
        return TeamPolicy(ni, std::min(128, 32*((nk+31)/32)));
    }
};
#endif

/**
    TeamUtils provide concurrency info for thread teams
*/
// template <typename ExeSpace=ko::DefaultExecutionSpace>
// class TeamUtils {
//     protected:
//         int _team_size;
//         int _num_teams;
//
//     public:
//         template <typename TeamPolicy>
//         TeamUtils(const TeamPolicy& policy) : _team_size(0) {
//             const int max_threads = ExeSpace::concurrency();
//             const int team_size = policy.team_size;
//         }
// };

}
#endif
