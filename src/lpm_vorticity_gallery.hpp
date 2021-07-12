#ifndef LPM_VORTICITY_GALLERY_HPP
#define LPM_VORTICITY_GALLERY_HPP

#include "LpmConfig.h"
#include "Kokkos_Core.hpp"
#include "lpm_constants.hpp"
#include "lpm_geometry.hpp"
#include "util/lpm_tuple.hpp"
#include "util/lpm_math_util.hpp"
#include "util/lpm_floating_point_util.hpp"
#include <memory>
#include <cmath>

namespace Lpm {

struct VorticityInitialCondition {
  typedef std::shared_ptr<VorticityInitialCondition> ptr;

  virtual ~VorticityInitialCondition() {}

  virtual Real operator() (const Real& x, const Real& y, const Real& z) const = 0;

  virtual Real operator() (const Real& x, const Real& y) const = 0;

  virtual std::string name() const {return std::string();}
};

struct SolidBodyRotation : public VorticityInitialCondition {
  static constexpr Real OMEGA = 2*PI;

  inline Real operator() (const Real& x, const Real& y, const Real& z) const {return 2*OMEGA*z;}

  inline Real operator() (const Real& x, const Real& y) const {return 0;}

  inline std::string name() const override {return "rotation";}

  inline void init_velocity(Real& u, Real& v, Real& w,
    const Real& x, const Real& y, const Real& z) const {
    u = -OMEGA*y;
    v =  OMEGA*x;
    w = 0;
  }
};

struct NitscheStricklandVortex : public VorticityInitialCondition {
  static constexpr Real b = 0.5;

  inline Real operator() (const Real& x, const Real& y, const Real& z) const {return 0;}

  inline Real operator() (const Real& x, const Real& y) const {
    const Real rsq = square(x) + square(y);
    const Real r = sqrt(rsq);
    return (3*safe_divide(r) - 2*b*r)*rsq*std::exp(-b*rsq);
  }

  inline std::string name() const {return "Nitsche&Strickland";}

  inline void init_velocity(Real& u, Real& v, const Real& x, const Real& y) const {
    const Real rsq = square(x) + square(y);
    const Real utheta = rsq*std::exp(-b*rsq);
    const Real theta = std::atan2(y,x);
    u = -utheta*std::sin(theta);
    v =  utheta*std::cos(theta);
  }
};

struct GaussianVortexSphere : public VorticityInitialCondition {
  Real gauss_const;
  Real vortex_strength;
  Real shape_parameter;
  Tuple<Real,3> xyz_ctr;

  GaussianVortexSphere(const Real str, const Real b, const Tuple<Real,3> ctr) :
    gauss_const(0),
    vortex_strength(str),
    xyz_ctr(ctr) {}

  void set_gauss_const(const Real vorticity_sum, const Index n_leaf_faces) {
    gauss_const = vorticity_sum / (4*constants::PI);
  }

  inline Real operator() (const Real& x, const Real& y, const Real& z) const {
    const Real distsq = 1.0 - x * xyz_ctr[0] - y * xyz_ctr[1] - z * xyz_ctr[2];
    return vortex_strength * exp(-square(shape_parameter)*distsq) - gauss_const;

  inline Real operator() (const Real& x, const Real& y) const {return 0;}

  inline std::string name() const {return "SphericalGaussianVortex";}

};

inline Real lamb_dipole_vorticity(const Real x, const Real y, const Real xctr, const Real yctr, const Real dipole_radius, const Real dipole_strength) {
  static constexpr Real LAMB_K0 = 3.8317;
  const Real r = sqrt(square(x-xctr) + square(y-yctr));
  Real result = 0;
  if ( (r < dipole_radius) and !FloatingPoint<T>::zero(r)) {
    const Real k = LAMB_K0 / dipole_radius;
    const Real sintheta = y/r;
    const Real denom = std::cyl_bessel_j(0, LAMB_K0);
    result = -2*dipole_strength * k * std::cyl_bessel_j(1, k*r)*sintheta/denom;
  }
  return result;
}

struct CollidingDipolePairPlane : VorticityInitialCondition {
  Real dipole_strengthA;
  Real dipole_radiusA;
  Tuple<Real,2> xyz_ctrA;
  Real dipole_strengthB;
  Real dipole_radiusB;
  Tuple<Real,2> xyz_ctrB;

  CollidingDipolePairPlane(const Real strA, const Real radA, const Tuple<Real,2> ctrA,
    const Real strB, const Real radB, const Tuple<Real,2> ctrB) :
    dipole_strengthA(strA),
    dipole_radiusA(radA),
    xyz_ctrA(ctrA),
    dipole_strengthA(strB),
    dipole_radiusA(radB),
    xyz_ctrA(ctrB) {}

  inline Real operator() (const Real& x, const Real& y, const Real& z) const {return 0;}

  inline Real operator() (const Real& x, const Real& y) const {
    return lamb_dipole_vorticity(x, y, xyz_ctrA[0], xyz_ctrA[1], dipole_radiusA, dipole_strengthA) +
    lamb_dipole_vorticity(x, y, xyz_ctrB[0], xyz_ctrB[1], dipole_radiusB, dipole_strengthB);
  }

  std::string name() const {return "PlanarCollidingDipoles";}
};

struct RossbyHaurwitz54 : VorticityInitialCondition {
  Real u0;
  Real rh54_amplitude;

  RossbyHaurwitz54(const Real zonal_background_velocity, const Real wave_amp) :
    u0(zonal_background_velocity),
    rh54_amplitude(wave_amp) {}

  inline Real operator() (const Real& x, const Real& y) const {return 0;}

  std::string name() const {return "RossbyHaurwitz54";}

  inline Real operator() (const Real& x, const Real& y const Real& z) const {
    const Real lon = atan4(y,x);
    return 2*u0*z + 30*rh54_amplitude*cos(4*lon) * std::legendre(5,z)*8.0/63.0;
  }

};



}
#endif