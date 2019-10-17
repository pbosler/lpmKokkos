unode_countunode_count#ifndef LPM_NODE_ARRAYD_HPP
#define LPM_NODE_ARRAYD_HPP

#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "LpmGeometry.hpp"
#include "LpmCoords.hpp"
#include "LpmBox3d.hpp"
#include "LpmOctreeUtil.hpp"
#include "LpmOctreeKernels.hpp"
#include "LpmKokkosUtil.hpp"
#include "Kokkos_Core.hpp"
#include "Kokkos_Sort.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

namespace Lpm {
namespace Octree {
/** 
    Implements Listing 1 from: 
    
    K. Zhou, et. al., 2011. Data-parallel octrees for surface reconstruction, 
    IEEE Trans. Vis. Comput. Graphics 17(5): 669--681. DOI: 10.1109/TVCG.2010.75 
*/

/**
    Node array at depth D
    Input: pts = kokkos view of 3d xyz coordinates
    
    Step 1: compute the bounding box
    
            BBox box;
            ko::parallel_reduce(pts.extent(0), BoxFunctor(pts), BBoxReducer<Space>(box));
        
    Step 2: compute the key of each point and encode the pt id with its key

            ko::parallel_for(pts.extent(0), KOKKOS_LAMBDA (const Index& i) {
                auto pos = ko::subview(pts, i, ko::ALL());
                const key_type key = compute_key_for_point(pos, D);
                codes(i) = encode(key, i);
            });
    
    Step 3: Sort by key, rearrange points into sorted order.
            a. sort the encoded key-id pairs by key
            
            ko::sort(codes);
            ko::View<Real*[3]> temp_pts("pts", pts.extent(0));
            
            b. rearrange pts array into sorted key order

            ko::parallel_for(pts.extent(0), PermuteFunctor(temp_pts, pts, codes));
            pts = temp_pts;
    
    Step 4: Consolidate unique nodes.
            a. Mark encoded keys as either unique (1) or duplicate (0).
    
            ko::parallel_for(ko::RangePolicy<MarkDuplicates::MarkTag>(0, pts.extent(0)), MarkDuplicates(flags, codes));
                    
            b. Find indices of the unique nodes and total number of unique nodes
                
            ko::parallel_scan(ko::RangePolicy<MarkDuplicates::ScanTag>(0, pts.extent(0)), MarkDuplicates(flags, codes));
                
            c. Make unique nodes and track their particle indices
            
            ko::parallel_for(pts.extent(0), NodeSetupFunctor(unique_keys, pt_inds, flags, codes));
            
    Step 5: Build NodeArrayD (1 of 2)
            a. Count nodes that need to be added.
            b. Calculate addresses of nodes to be added.
    
    Step 6: Build NodeArrayD (2 of 2)
        
        
    
*/
class NodeArrayD {
    public:
    ko::View<Real*[3]> sorted_pts; /// point coordinates in R3 (input).
//    Int level; /// depth of this node array in the octree (input).
    Int depth; /// maximum depth of octree.
    
    ko::View<BBox> box; /// Bounding box
    ko::View<key_type*> node_keys; /// node_keys(i) = shuffled xyz key of node i
    ko::View<Index*> node_pt_idx; /// node_pt_idx(i) = index of first point contained in node i
    ko::View<Index*> node_pt_ct; /// node_pt_ct(i) = number of points contained in node i
    ko::View<Index*> node_parent; /// allocated here; set by level D-1
    
    ko::View<Index*> pt_in_node; /// pt_in_node(i) = index of the node that contains point i
    ko::View<Index*> orig_ids; /// original (presort) locations of points
        
    NodeArrayD(const ko::View<Real*[3]>& p, const Int& d) : depth(d), sorted_pts("sorted_points", p.extent(0)),
        pt_in_node("pt_in_node", p.extent(0)), orig_ids("original_pt_locs", p.extent(0)), box("bbox") {init(p);}
    
    /**
        Listing 1:  Initializer for lowest level of octree
    */
    void init(const ko::View<Real*[3]>& presorted_points) {
        assert(depth > 0 && depth <= MAX_OCTREE_DEPTH);
        const Index npts = presorted_points.extent(0);
        /// step 1: Determine bounding box of point set
        /**
            Input: points
            Process: 
                Kernel: BoxFunctor
                    Reduction
                    Loop over: Points
            Output: bounding box
        */
        ko::parallel_reduce(npts, BoxFunctor(presorted_points), BBoxReducer<Dev>(box));
        
        /// step 2: Encode node key/point index pairs
        /**
            Input: points
            Process: compute 32-bit node key of each point, concatenate 32-bit point id into 64-bit code
            Kernel : EncodeFunctor
            Loop over: points
            Output: encoded point key/id pairs
            Sort code array
        */
        ko::View<code_type*> pt_codes("pt_codes",npts);
        ko::parallel_for(npts, EncodeFunctor(pt_codes, presorted_points, box, depth));
        ko::sort(pt_codes);

        /// step 3: Sort point array
        /**
            Input: points, sorted point codes
            Process: rearrange points according to sorted codes.
                Kernel: PermuteFunctor
                    Loop over: codes, points
            Output: sorted point array, with original id array to allow unsort later
        */
        ko::parallel_for(npts, PermuteFunctor(sorted_pts, orig_ids, presorted_points, pt_codes));

        /// step 4: Determine points contained by each node
        /**
            Input: point codes
            Process: 
                Kernel: MarkDuplicates
                    Loop over: point codes                
                    a. Flag each unique node using encoded keys (flag = 1 for unique node; 0 otherwise)
                    b. Scan to count the number of unique nodes 
                    c. After scan, flags(npts-1) = count of nodes that have points
                    
                Kernel: UniqueNodeFunctor
                    Loop over: point codes
                    For each node, find index of first contained point, number of points contained
            Output: 
                node keys 
                point start, count
        */
        ko::View<Index*> node_flags("node_flags",npts);
        ko::parallel_for(ko::RangePolicy<MarkDuplicates::MarkTag>(0,npts), 
            MarkDuplicates(node_flags, pt_codes));
        Index unode_count = 0;
        ko::parallel_for(ko::RangePolicy<MarkDuplicates::SumTag>(0,npts),
            MarkDuplicates(node_flags, pt_codes), unode_count);
        ko::parallel_scan(ko::RangePolicy<MarkDuplicates::ScanTag>(0,npts),
            MarkDuplicates(node_flags, pt_codes));
        
        ko::View<key_type*> ukeys("keys", unode_count);
        ko::View<Index*[2]> pt_inds("pt_inds", unode_count);
        ko::parallel_for(npts, UniqueNodeFunctor(ukeys, pt_inds, node_flags, pt_codes));
        
        /// step 5: Ensure that each node has a full set of siblings
        /**
            Input: keys for nodes that contain points
            Process:
                Kernel: NodeAddressFunctor
                    Loop over: unique nodes
                    a. Mark node keys that have different parents
                    b. Scan to find starting address of each set of siblings
            Output:
                node_address contains starting point for each set of 8 siblings
                node_address(-1) + 8 = number of nodes at this level (nodes with data and possibly empty siblings)
        */
        ko::View<Index*> node_address("node_address", ukeys.extent(0));
        ko::parallel_for(ko::RangePolicy<NodeAddressFunctor::MarkTag>(0, ukeys.extent(0)),
            NodeAddressFunctor(node_address, ukeys, depth, depth));
        Index nnodes = 0;
        ko::parallel_reduce(ko::RangePolicy<NodeAddressFunctor::SumTag>(0, ukeys.extent(0)),
            NodeAddressFunctor(node_address, ukeys, depth, depth));
        ko::parallel_scan(ko::RangePolicy<NodeAddressFunctor::ScanTag>(0, ukeys.extent(0)),
            NodeAddressFunctor(node_address, ukeys, depth, depth));
        
        /// step 6: Make NodeArrayD
        /**
            Input:
            Process:
                Loop over: unique nodes
                Kernel: NodeSetupFunctor
                    If a new parent is found, reserve space for its 8 children
                    Add keys for each child
                Loop over: unique nodes
                Kernel: NodeFillFunctor
                    Fill node with point start index and point counts
                    Tag point with address of node that contains it
            Output:
                All nodes at level are added, including empty siblings
                Nodes containing points have start indices, counts filled
                Points know which node contains them.
        */
        node_count = ko::subview(node_address, node_count_host()-1);
        ko::deep_copy(node_count_host, node_count);
        const key_type nnodes = node_count_host() + 8;
        std::ostringstream ss;
        ss << "node_keys" << depth;
        node_keys = ko::View<key_type*>(ss.str(), nnodes);
        ss.str("");
        ss << "pt_start_index" << depth;
        node_pt_idx = ko::View<Index*>(ss.str(), nnodes);
        ss.str("");
        ss << "pt_count" << depth;
        node_pt_ct = ko::View<Index*>(ss.str(), nnodes);
        ss.str("");
        ss << "node_parent" << depth;
        node_parent = ko::View<Index*>(ss.str(), nnodes);

        auto policy6a = ExeSpaceUtils<>::get_default_team_policy(ukeys.extent(0), 8);
        ko::parallel_for(policy6a, NodeSetupFunctor(node_keys, node_address, ukeys, depth, depth));

        auto policy6b = ExeSpaceUtils<>::get_default_team_policy(ukeys.extent(0), 128);
        ko::parallel_for(policy6b, NodeFillFunctor(node_pt_idx, node_pt_ct, pt_in_node, ukeys, 
            node_address, pt_inds, depth, depth));
    };
    
    std::string infoString() const;
};


}}
#endif
