#ifndef LPM_OCTREE_KERNELS_HPP
#define LPM_OCTREE_KERNELS_HPP

#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "LpmUtilities.hpp"
#include "LpmKokkosUtil.hpp"
#include "LpmBox3d.hpp"
#include "LpmOctreeUtil.hpp"

#include "Kokkos_Core.hpp"

#include <cstdint>
#include <iostream>
#include <cassert>
#include <cmath>

namespace Lpm {
namespace Octree {

/**
    Compute shuffled xyz key for a point, concatenate point id with key.
    
    Loop over: Points
*/
struct EncodeFunctor {
    // output
    ko::View<code_type*> codes;
    // input
    ko::View<Real*[3]> pts;
    ko::View<BBox> box;
    Int depth;
    
    EncodeFunctor(ko::View<code_type*>& co, const ko::View<Real*[3]>& p, const ko::View<BBox>& b, const Int& md) :
        codes(co), pts(p), box(b), depth(md) {}
        
    KOKKOS_INLINE_FUNCTION
    void operator() (const Index& i) const {
        // each thread i gets a point
        auto pos = ko::subview(pts, i, ko::ALL());
        const key_type key = compute_key_for_point(pos, depth, box());
        codes(i) = encode(key, i);
    }
};

/**
    Using sorted codes (input), move points into sorted order.
    
    Loop over: point codes
*/
struct PermuteFunctor {
    // output
    ko::View<Real*[3]> outpts;
    ko::View<Index*> orig_inds;
    // input
    ko::View<Real*[3]> inpts;
    ko::View<code_type*> codes;
    
    PermuteFunctor(ko::View<Real*[3]>& op, ko::View<Index*>& oi, const ko::View<Real*[3]>& ip, 
        const ko::View<code_type*>& c) :  outpts(op), orig_inds(oi), inpts(ip), codes(c) {}
        
    KOKKOS_INLINE_FUNCTION
    void operator() (const id_type& i) const {
        // Each thread gets a point code
        const id_type old_id = decode_id(codes(i));
        orig_inds(i) = old_id;
        for (int j=0; j<3; ++j) {
            outpts(i,j) = inpts(old_id,j);
        }
    }
};

struct UnpermuteFunctor {
    // output 
    ko::View<Real*[3]> outpts;
    // input
    ko::View<Real*[3]> inpts;
    ko::View<Index*> old_id;
    
    UnpermuteFunctor(ko::View<Real*[3]>& op, const ko::View<Real*[3]>& ip, const ko::View<Index*>& oi) :
        outpts(op), inpts(ip), old_id(oi) {}
    
    KOKKOS_INLINE_FUNCTION
    void operator() (const Index& i) const {
        for (int j=0; j<3; ++j) {
            outpts(old_id(i),j) = inpts(i,j);
        }
    }
};


/**
    Flag, Inclusive Scan Functor.
    
    Step 1: Flag 
        Loop over sorted codes.  Set flag = 1 if new node key is found, 0 otherwise.
    Step 2: Scan
        Scan flags (inclusive)
        After scan, flag(npts-1)+1 = number of unique nodes.
    
*/
struct MarkDuplicates {
    // output
    ko::View<Index*> flags;
    // input
    ko::View<code_type*> codes;
    
    MarkDuplicates(ko::View<Index*> f, const ko::View<code_type*>& c) : flags(f), codes(c) {}
    
    struct MarkTag {};
    struct ScanTag {};
    
    KOKKOS_INLINE_FUNCTION
    void operator () (const MarkTag&, const Index& i) const {
        if (i > 0) {
            flags(i) = (decode_key(codes(i)) != decode_key(codes(i-1)));
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
            flags(i) = ct;
        }        
    }
};

/**
    Collect data about each unique node, to be used later to construct the nodes in NodeArray.
    
    input = output of MarkDuplicates kernel's 2 steps.
    
    output = array containing unique node keys
        node_ind = flag(i) if flag(i) is a new node.  Otherwise, the thread is idle.
    for each node key, 2 indices:
        inds_out(node_ind, 0) = index of first point (in sorted_pts) contained by node
        inds_out(node_ind, 1) = count of points contained by node
*/
struct UniqueNodeFunctor {
    // output
    ko::View<key_type*> keys_out;
    ko::View<Index*[2]> inds_out;
    
    // input
    ko::View<Index*> flags;
    ko::View<code_type*> codes_in;
    
    UniqueNodeFunctor(ko::View<key_type*>& oc, ko::View<Index*[2]>& io,
    	const ko::View<Index*>& f, const ko::View<code_type*>& ic) : 
        flags(f), codes_in(ic), keys_out(oc), inds_out(io) {}
    
    KOKKOS_INLINE_FUNCTION
    void operator () (const Index& i) const {
        // Each thread gets an index into flags
        bool newnode = true;
        if (i > 0) newnode = (flags(i) > flags(i-1));
        if (newnode) {
            // thread finds a new node
        	const Index node_ind = flags(i)-1;
        	const key_type newkey = decode_key(codes_in(i));
            keys_out(node_ind) = newkey;
            const Index first = binarySearchCodes(newkey, codes_in, true);
            const Index last = binarySearchCodes(newkey, codes_in, false);
            inds_out(node_ind,0) = first;
            inds_out(node_ind,1) = last - first + 1;
        }
        // else thread is idle
    }
};


/**
    For later nearest-neighbor searches, we want to add the siblings of every unique node, even if they're empty,
    so that each parent will have a full set of 8 child nodes.
    
    2-step Flag, Scan
    
    Loop over: unique nodes
    
    Step 1: Flag
    For thread i, where i>0:
        If thread i and thread i-1 have the same parent, flag node_num = 0
        If thread i and thread i-1 have different parents, flag node_num = 8
    Step 3: Scan (inclusive)
        
*/
struct NodeSiblingCounter {
    // output
	ko::View<Index*> nsiblings;
	
	// input
	ko::View<key_type*> keys_in;
	Int lev;
	Int max_depth;
	
	NodeSiblingCounter(ko::View<Index*> na, const ko::View<key_type*>& kk, const Int& ll, const Int& md) :
        nsiblings(na), keys_in(kk), lev(ll), max_depth(md) {}
	
	struct MarkTag {};
	struct ScanTag {};
	
	KOKKOS_INLINE_FUNCTION
	void operator () (const MarkTag&, const Index& i) const {
		if (i>0) {
			const key_type pt_i = parent_key(keys_in(i), lev, max_depth);
			const key_type pt_im1 = parent_key(keys_in(i-1), lev, max_depth);
            nsiblings(i) = (pt_i == pt_im1 ? 0 : 8);
		}
		else {
		    nsiblings(i) = 8;
		}
	}

	KOKKOS_INLINE_FUNCTION
	void operator() (const ScanTag&, const Index& i, Index& ct, const bool& final_pass) const {
		const Index inc = nsiblings(i);
        ct += inc;
		if (final_pass) {
			nsiblings(i) = ct;
		}		
	}
};

/**
    Prepare NodeArrayD construction.
    
    // input
    
    // output
*/
struct NodeSetupFunctor {
    ko::View<key_type*> keys_out;
    ko::View<Index*> node_address;
    ko::View<key_type*> keys_in;
    Int level;
    Int max_depth;
    
    NodeSetupFunctor(ko::View<key_type*>& ko, const ko::View<Index*>& na, const ko::View<key_type*>& ki,
         const Int& lev, const Int& md) :
        keys_out(ko), node_address(na), keys_in(ki), level(lev), max_depth(md) {}

    KOKKOS_INLINE_FUNCTION
    void operator() (const member_type& mbr) const {
        const Index i=mbr.league_rank();
        bool new_parent = true;
        if (i>0) {
            new_parent = node_address(i) > node_address(i-1);
        }
        if (new_parent) {
            const key_type pkey = parent_key(keys_in(i), level, max_depth);
            ko::parallel_for(ko::TeamThreadRange(mbr, (level>0 ? 8 : 1)), KOKKOS_LAMBDA (const Int& j) {
                keys_out(node_address(i)+j) = node_key(pkey, j, level, max_depth);
            });
        }
    }
};

struct NodeFillFunctor {
    // output
    ko::View<Index*> pt_idx;
    ko::View<Index*> pt_ct;
    ko::View<Index*> pt_in_node;
    
    // input
    ko::View<key_type*> keys_in;  // only non-empty keys
    ko::View<Index*> node_address;
    ko::View<Index*[2]> pt_inds;    
    Int level;
    Int max_depth;
    
    NodeFillFunctor(ko::View<Index*>& ps, ko::View<Index*>& pc, ko::View<Index*>& pn,
        const ko::View<key_type*>& ki, const ko::View<Index*>& na, const ko::View<Index*[2]>& pi, 
        const Int& ll, const Int& md=MAX_OCTREE_DEPTH) : pt_idx(ps), pt_ct(pc), pt_in_node(pn),
        keys_in(ki), node_address(na), pt_inds(pi), level(ll), max_depth(md) {}
    
    KOKKOS_INLINE_FUNCTION
    void operator() (const member_type& mbr) const {
    	const Index i = mbr.league_rank();
    	const key_type address = node_address(i) + local_key(keys_in(i), level, max_depth);
    	pt_idx(address) = pt_inds(i,0);
    	pt_ct(address) = pt_inds(i,1);
    	ko::parallel_for(ko::TeamThreadRange(mbr, pt_inds(i,0), pt_inds(i,0)+pt_inds(i,1)),
    		KOKKOS_LAMBDA (const Index& j) {
    			pt_in_node(j) = address;
    		});
    }
};

}}
#endif
