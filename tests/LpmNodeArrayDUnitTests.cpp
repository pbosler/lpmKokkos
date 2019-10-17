#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "LpmBox3d.hpp"
#include "LpmKokkosUtil.hpp"
#include "LpmOctreeUtil.hpp"
#include "LpmNodeArrayD.hpp"
#include "LpmNodeArrayInternal.hpp"
#include "LpmPolyMesh2d.hpp"
#include <fstream>
using namespace Lpm;
using namespace Octree;

int main(int argc, char* argv[]) {
ko::initialize(argc, argv);
{
    {
    const int npts = 6;
    const int max_depth = 4;
    const int tree_lev = 3;
    ko::View<Real*[3]> pts("pts",npts);
    typename ko::View<Real*[3]>::HostMirror host_pts = ko::create_mirror_view(pts);
    for (int i=0; i<4; ++i) {
        host_pts(i,0) = (i%2==0 ? 2.0*i : -i);
        host_pts(i,1) = (i%2==1 ? 2-i : 1 + i);
        host_pts(i,2) = (i%2==0 ? i : -i);
    }
    host_pts(4,0) = 0.01;
    host_pts(4,1) = 0.9;
    host_pts(4,2) = 0.01;
    host_pts(5,0) = 0.011;
    host_pts(5,1) = 0.91;
    host_pts(5,2) = 0.015;
    ko::deep_copy(pts, host_pts);
    
    std::cout << "points ready.\n";
    
    NodeArrayD leaves(pts, tree_lev);
    
    NodeArrayInternal nextlev(leaves);
    
    NodeArrayInternal topLevel(nextlev);
    
    NodeArrayInternal root(topLevel);
    std::cout << root.infoString();
    std::cout << topLevel.infoString();
    std::cout << nextlev.infoString();
    std::cout << leaves.infoString();
    
    }
    
    {
    /**
        Build source mesh 
    */
    const int mesh_depth = 1;
    const int octree_depth = mesh_depth;
    Index nmaxverts, nmaxedges, nmaxfaces;
    MeshSeed<IcosTriSphereSeed> icseed;
    icseed.setMaxAllocations(nmaxverts, nmaxedges, nmaxfaces, mesh_depth);
    PolyMesh2d<SphereGeometry,TriFace> trisphere(nmaxverts, nmaxedges, nmaxfaces);
    trisphere.treeInit(mesh_depth, icseed);
    trisphere.updateDevice();
    ko::View<Real*[3]> src_crds = sourceCoords<SphereGeometry,TriFace>(trisphere);

    NodeArrayD leaves(src_crds, octree_depth);
    auto leaf_keys_host = ko::create_mirror_view(leaves.node_keys);
    auto src_host = ko::create_mirror_view(src_crds);
    auto leaf_pt_start = ko::create_mirror_view(leaves.node_pt_idx);
    auto leaf_pt_ct = ko::create_mirror_view(leaves.node_pt_ct);
    auto rbox_host = ko::create_mirror_view(leaves.box);
    auto pt_in_node_host = ko::create_mirror_view(leaves.pt_in_node);
    ko::deep_copy(rbox_host, leaves.box);
    ko::deep_copy(src_host, src_crds);
    ko::deep_copy(leaf_keys_host, leaves.node_keys);
    ko::deep_copy(leaf_pt_start, leaves.node_pt_idx);
    ko::deep_copy(leaf_pt_ct, leaves.node_pt_ct);
    ko::deep_copy(pt_in_node_host, leaves.pt_in_node);
    std::cout << "SPHERE TEST\n";
    std::cout << "\troot box = " << rbox_host();
    for (Index i=0; i<leaf_keys_host.extent(0); ++i) {
        BBox node_box = box_from_key(leaf_keys_host(i),rbox_host(), octree_depth, octree_depth);
        for (Index j=0; j<leaf_pt_ct(i); ++j) {
            if (!boxContainsPoint(node_box, ko::subview(src_host, leaf_pt_start(i)+j, ko::ALL()))) {
                std::cout << "Found error: box " << node_box;
                std::cout << "\t does not contain point " << leaf_pt_start(i)+j << " (";
                for (int k=0; k<3; ++k) {
                    std::cout << src_host(leaf_pt_start(i)+j,k) << " ";
                }
                std::cout << ")\n";
            }
        }
    }
    std::ofstream of("node_array_d_test_output.txt");
    for (Index i=0; i<src_host.extent(0); ++i) {
        std::cout << "point(" << i << ") = (";
        for (int j=0; j<3; ++j) {
            std::cout << src_host(i,j) << (j!=2 ? " " : ") ");
        }
        std::cout << "is in node " << pt_in_node_host(i);
        const auto nbox = box_from_key(leaf_keys_host(pt_in_node_host(i)), rbox_host(), octree_depth, octree_depth);
        const bool pt_in_box = boxContainsPoint(nbox, ko::subview(src_host, i, ko::ALL()));
        std::cout << "; node's box contains pt = " << std::boolalpha << pt_in_box
            << nbox;
    }
    
//     NodeArrayInternal nextlev(leaves);
    
    of << leaves.infoString();
//     of << nextlev.infoString();
    of.close();
    }
    std::cout << "program complete." << std::endl;
}
ko::finalize();
return 0;
}