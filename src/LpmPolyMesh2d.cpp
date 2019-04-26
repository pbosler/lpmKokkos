#include "LpmPolyMesh2d.hpp"
#include "LpmKokkosUtil.hpp"
#include "LpmVtkIO.hpp"

namespace Lpm {

template <typename Geo, typename FaceType> template <typename SeedType>
void PolyMesh2d<Geo,FaceType>::seedInit(const MeshSeed<SeedType>& seed) {
    physVerts.initBoundaryCrdsFromSeed(seed);
    lagVerts.initBoundaryCrdsFromSeed(seed);
    edges.initFromSeed(seed);
    faces.initFromSeed(seed);
    physFaces.initInteriorCrdsFromSeed(seed);
    lagFaces.initInteriorCrdsFromSeed(seed);
}

template <typename Geo, typename FaceType> template <typename SeedType>
void PolyMesh2d<Geo,FaceType>::treeInit(const Int initDepth, const MeshSeed<SeedType>& seed) {
    seedInit<SeedType>(seed);
    for (int i=0; i<initDepth; ++i) {
        Index startInd = 0;
        Index stopInd = faces.nh();
        for (Index j=startInd; j<stopInd; ++j) {
            if (!faces.hasKidsHost(j)) {
                divider::divide(j, faces, edges, physFaces, lagFaces, physVerts, lagVerts);
            }
        }
    }
}

template <typename Geo, typename FaceType> 
void PolyMesh2d<Geo,FaceType>::outputVtk(const std::string& fname) const {
    VtkInterface<Geo,Faces<FaceType>> vtk;
    vtk.toVtkPolyData(faces, edges, physFaces, physVerts);
    vtk.writePolyData(fname);
}


/// ETI
template class PolyMesh2d<PlaneGeometry,TriFace>;
template class PolyMesh2d<PlaneGeometry,QuadFace>;
template class PolyMesh2d<SphereGeometry,TriFace>;
template class PolyMesh2d<SphereGeometry,QuadFace>;

template void PolyMesh2d<PlaneGeometry,TriFace>::treeInit(const Int initDepth, const MeshSeed<TriHexSeed>& seed);
template void PolyMesh2d<PlaneGeometry,QuadFace>::treeInit(const Int initDepth, const MeshSeed<QuadRectSeed>& seed);
template void PolyMesh2d<SphereGeometry,TriFace>::treeInit(const Int initDepth, const MeshSeed<IcosTriSphereSeed>& seed);
template void PolyMesh2d<SphereGeometry,QuadFace>::treeInit(const Int initDepth, const MeshSeed<CubedSphereSeed>& seed);



}
