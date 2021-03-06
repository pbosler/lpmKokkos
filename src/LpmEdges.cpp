#include "LpmEdges.hpp"
#include "LpmUtilities.hpp"

#ifdef LPM_HAVE_NETCDF
#include "LpmNetCDF.hpp"
#endif

#include <sstream>

namespace Lpm {

void Edges::insertHost(const Index o, const Index d, const Index l, const Index r, const Index prt) {
  LPM_THROW_IF(_nmax < _nh() + 1, infoString("Edges::insertHost error: not enough memory."));
  const Index ins_pt = _nh();
  _ho(ins_pt) = o;
  _hd(ins_pt) = d;
  _hl(ins_pt) = l;
  _hr(ins_pt) = r;
  _hp(ins_pt) = prt;
  _hk(ins_pt, 0) = NULL_IND;
  _hk(ins_pt, 1) = NULL_IND;
  _nh() += 1;
  _hnLeaves() += 1;
}

#ifdef LPM_HAVE_NETCDF
Edges::Edges(const PolyMeshReader& reader) : Edges(reader.nEdges()) {
  reader.fill_origs(_ho);
  reader.fill_dests(_hd);
  reader.fill_lefts(_hl);
  reader.fill_rights(_hr);
  reader.fill_edge_tree(_hp, _hk, _hnLeaves());
  _nh() = origs.extent(0);
  updateDevice();
  }
#endif

template <typename SeedType>
void Edges::initFromSeed(const MeshSeed<SeedType>& seed) {
  LPM_THROW_IF(_nmax < SeedType::nedges, "Edges::initFromSeed error: not enough memory.");
  for (Int i=0; i<SeedType::nedges; ++i) {
    _ho(i) = seed.sedges(i,0);
    _hd(i) = seed.sedges(i,1);
    _hl(i) = seed.sedges(i,2);
    _hr(i) = seed.sedges(i,3);
    _hp(i) = NULL_IND;
    _hk(i,0) = NULL_IND;
    _hk(i,1) = NULL_IND;
  }
  _nh() = SeedType::nedges;
  _hnLeaves() = SeedType::nedges;
}


template <typename Geo> void Edges::divide(const Index ind, Coords<Geo>& crds, Coords<Geo>& lagcrds) {
  assert(ind < _nh());

  LPM_THROW_IF(_nh() + 2 > _nmax, "Edges::divide error: not enough memory.");
  LPM_THROW_IF(hasKidsHost(ind), "Edges::divide error: called on previously divided edge.");
  // record beginning state
  const Index crd_ins_pt = crds.nh();
  const Index edge_ins_pt = _nh();

  // determine edge midpoints
  ko::View<Real[Geo::ndim], Host> midpt("midpt"), lagmidpt("lagmidpt");
  ko::View<Real[2][Geo::ndim], Host> endpts("endpts"), lagendpts("lagendpts");
  for (int i=0; i<Geo::ndim; ++i) {
    endpts(0,i) = crds.getCrdComponentHost(_ho(ind), i);
    lagendpts(0,i) = lagcrds.getCrdComponentHost(_ho(ind), i);
    endpts(1,i) = crds.getCrdComponentHost(_hd(ind), i);
    lagendpts(1,i) = lagcrds.getCrdComponentHost(_hd(ind), i);
  }

  Geo::midpoint(midpt, ko::subview(endpts, 0, ko::ALL()), ko::subview(endpts, 1, ko::ALL()));
  Geo::midpoint(lagmidpt, ko::subview(lagendpts, 0, ko::ALL()), ko::subview(lagendpts, 1, ko::ALL()));
  // insert new midpoint to Coords
  crds.insertHost(midpt);
  lagcrds.insertHost(lagmidpt);
  // insert new child edges
  insertHost(_ho(ind), crd_ins_pt, _hl(ind), _hr(ind), ind);
  insertHost(crd_ins_pt, _hd(ind), _hl(ind), _hr(ind), ind);
  _hk(ind,0) = edge_ins_pt;
  _hk(ind,1) = edge_ins_pt+1;
  _hnLeaves() -= 1;
}

template <> void Edges::divide<CircularPlaneGeometry>(const Index ind,
  Coords<CircularPlaneGeometry>& crds, Coords<CircularPlaneGeometry>& lagcrds) {

  assert(ind < _nh());

  LPM_THROW_IF(_nh()+2 > _nmax, "Edges::divide error: not enough memory.");
  LPM_THROW_IF(hasKidsHost(ind), "Edges::divide error: called on previously divided edge.");

  // record starting state
  const Index crd_ins_pt = crds.nh();
  const Index edge_ins_pt = _nh();

  // determine edge midpoints
  ko::View<Real[2], Host> midpt("midpt"), lagmidpt("lagmidpt");
  ko::View<Real[2][2],Host> endpts("endpts"), lagendpts("lagendpts");
  for (Short i=0; i<2; ++i) {
    endpts(0,i) = crds.getCrdComponentHost(_ho(ind), i);
    lagendpts(0,i) = lagcrds.getCrdComponentHost(_ho(ind),i);
    endpts(1,i) = crds.getCrdComponentHost(_hd(ind), i);
    lagendpts(1,i) = lagcrds.getCrdComponentHost(_hd(ind),i);
  }
  const Real lr0 = CircularPlaneGeometry::mag(ko::subview(lagendpts,0,ko::ALL()));
  const Real lr1 = CircularPlaneGeometry::mag(ko::subview(lagendpts,1,ko::ALL()));
  if (fp_equiv(lr0, lr1, 10*ZERO_TOL)) {
    CircularPlaneGeometry::radial_midpoint(midpt, ko::subview(endpts, 0, ko::ALL()),
      ko::subview(endpts,1,ko::ALL()));
    CircularPlaneGeometry::radial_midpoint(lagmidpt, ko::subview(lagendpts, 0, ko::ALL()),
      ko::subview(lagendpts,1,ko::ALL()));
  }
  else {
    CircularPlaneGeometry::midpoint(midpt, ko::subview(endpts, 0, ko::ALL()),
      ko::subview(endpts,1,ko::ALL()));
    CircularPlaneGeometry::midpoint(lagmidpt, ko::subview(lagendpts, 0, ko::ALL()),
      ko::subview(lagendpts,1,ko::ALL()));
  }
  crds.insertHost(midpt);
  lagcrds.insertHost(lagmidpt);

  insertHost(_ho(ind), crd_ins_pt, _hl(ind), _hr(ind), ind);
  insertHost(crd_ins_pt, _hd(ind), _hl(ind), _hr(ind), ind);
  _hk(ind,0) = edge_ins_pt;
  _hk(ind,1) = edge_ins_pt+1;
  _hnLeaves() -= 1;
}

std::string Edges::infoString(const std::string& label, const short& tab_level, const bool& dump_all) const {
  std::ostringstream oss;
  const auto indent = indentString(tab_level);

  oss << indent << "Edges " << label << " info: nh = (" << _nh() << ") of nmax = " << _nmax << " in memory; "
    << _hnLeaves() << " leaves." << std::endl;

  if (dump_all) {
    const auto bigindent = indentString(tab_level+1);
    for (Index i=0; i<_nmax; ++i) {
      if (i==_nh()) oss << indent << "---------------------------------" << std::endl;
      oss << bigindent << label << ": (" << i << ") : ";
      oss << "orig = " << _ho(i) << ", dest = " << _hd(i);
      oss << ", left = " << _hl(i) << ", right = " << _hr(i);
      oss << ", parent = " << _hp(i) << ", kids = " << _hk(i,0) << "," << _hk(i,1);
      oss << std::endl;
    }
  }
  return oss.str();
}

/// ETI
template void Edges::divide<PlaneGeometry>(const Index ind, Coords<PlaneGeometry>& crds, Coords<PlaneGeometry>& lagcrds);

template void Edges::divide<SphereGeometry>(const Index ind, Coords<SphereGeometry>& crds, Coords<SphereGeometry>& lagcrds);

template void Edges::initFromSeed(const MeshSeed<TriHexSeed>& seed);
template void Edges::initFromSeed(const MeshSeed<QuadRectSeed>& seed);
template void Edges::initFromSeed(const MeshSeed<CubedSphereSeed>& seed);
template void Edges::initFromSeed(const MeshSeed<IcosTriSphereSeed>& seed);
template void Edges::initFromSeed(const MeshSeed<UnitDiskSeed>& seed);

}
