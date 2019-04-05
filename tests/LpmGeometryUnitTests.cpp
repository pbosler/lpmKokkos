#include "LpmConfig.h"
#include "LpmDefs.hpp"
#include "LpmGeometry.hpp"
#include <typeinfo>
#include <fstream>
#include <string>

using namespace Lpm;

typedef ko::DefaultExecutionSpace ExeSpace;

int main(int argc, char* argv[]) {
ko::initialize(argc, argv); 
{
    { //PLANAR TESTS
    const Real r2a[2] = {1.0, 0.0};
    const Real r2b[2] = {2.0, 1.0};
    const Real r2c[2] = {-1.0, -10.0};
    const Real r2d[2] = {-0.5, -1};
    
    
    // allocate a Vec on the device
    Vec<2, ExeSpace> r2device_a("a");
    // allocate a mirror on host
    Vec<2, ExeSpace>::HostMirror ha = ko::create_mirror_view(r2device_a);
    // initialize on host
    ha[0] = r2a[0];
    ha[1] = r2a[1];
    // copy to device
    ko::deep_copy(r2device_a, ha);
    
    Vec<2, ExeSpace> r2device_b("b");
    Vec<2, ExeSpace>::HostMirror hb = ko::create_mirror_view(r2device_b);
    hb[0] = r2b[0];
    hb[1] = r2b[1];
    ko::deep_copy(r2device_b, hb);
    
    Vec<2, ExeSpace> r2device_c("c");
    Vec<2, ExeSpace>::HostMirror hc = ko::create_mirror_view(r2device_c);
    hc[0] = r2c[0];
    hc[1] = r2c[1];
    ko::deep_copy(r2device_c, hc);
    
    Vec<2, ExeSpace> r2device_d("d");
    Vec<2, ExeSpace>::HostMirror hd = ko::create_mirror_view(r2device_d);
    hd[0] = r2d[0];
    hd[1] = r2d[1];
    ko::deep_copy(r2device_d, hd);
    
    // allocate a scalar on the device and a host mirror
    ko::View<Real,Layout,ExeSpace> scalar_result("res");
    ko::View<Real>::HostMirror host_scalar = ko::create_mirror_view(scalar_result);
    Vec<2, ExeSpace> result("res");
    
    
    Vec<2, ExeSpace>::HostMirror host_result = ko::create_mirror_view(result);
    VecArr<2, ExeSpace> vecs("vecs", 4);
    std::cout << "shape(vecs) = (" << vecs.extent(0) << ", " << vecs.extent(1) << ")" << std::endl;
    
    std::cout << typeid(vecs).name() << std::endl;
    
    // execute functions on device
    ko::parallel_for(1, KOKKOS_LAMBDA (int i) {
        scalar_result(0) = PlaneGeometry::triArea<Vec<2,ExeSpace>>(r2device_a, r2device_b, r2device_c);
        if (scalar_result(0) != 4) error("triArea error");
        for (int j=0; j<2; ++j) {
            vecs(0,j) = r2device_a(j);
            vecs(1,j) = r2device_b(j);
            vecs(2,j) = r2device_c(j);
            vecs(3,j) = r2device_d(j);
        }
        PlaneGeometry::midpoint(result, slice(vecs, 0), slice(vecs, 1));
        if (result(0) != 1.5 || result(1) != 0.5) error("midpt error\n");
        PlaneGeometry::barycenter(result, vecs, 3);
        if (result(0) != 2.0/3.0 || result(1) != -3) error("barycenter error\n");
        scalar_result(0) = PlaneGeometry::distance(slice(vecs, 0), slice(vecs,2));
        if ( std::abs(scalar_result(0) - 2*std::sqrt(26.0)) > ZERO_TOL) error("distance error\n");
        scalar_result(0) = PlaneGeometry::polygonArea(vecs, 4);
        if (std::abs(scalar_result(0) - 10.5) > ZERO_TOL) error("polygonArea error.");
        scalar_result(0) = PlaneGeometry::dot(r2device_a, r2device_b);
        }); 
    // copy results to host
    ko::deep_copy(host_result, result);
    prarr("barycenter", host_result.data(), 2);
    LPM_THROW_IF( (host_result(0) != 2.0/3.0 || host_result(1) != -3), "barycenter error\n");
    ko::deep_copy(host_scalar, scalar_result);
    LPM_THROW_IF( host_scalar(0) != 2, "dot product error.\n");
    PlaneGeometry::normalize(host_result);
    LPM_THROW_IF( PlaneGeometry::mag(host_result) != 1, "normalize error.");
    } // END PLANAR TESTS
    
    { // SPHERICAL TESTS
    
    const Real p0[3] = {0.57735026918962584,-0.57735026918962584,0.57735026918962584};
    const Real p1[3] = {0.57735026918962584,-0.57735026918962584,-0.57735026918962584};
    const Real p2[3] = {0.57735026918962584,0.57735026918962584,-0.57735026918962584};
    const Real p3[3]= {0.57735026918962584,0.57735026918962584,0.57735026918962584};
    
    Vec<3, ExeSpace> a("a");
    Vec<3, ExeSpace>::HostMirror ha = ko::create_mirror_view(a);
    Vec<3, ExeSpace> b("b");
    Vec<3, ExeSpace>::HostMirror hb = ko::create_mirror_view(b);
    Vec<3, ExeSpace> c("c");
    Vec<3, ExeSpace>::HostMirror hc = ko::create_mirror_view(c);
    Vec<3, ExeSpace> d("d");
    Vec<3, ExeSpace>::HostMirror hd = ko::create_mirror_view(d);
    
    for (int i=0; i<3; ++i) {
        ha[i] = p0[i];
        hb[i] = p1[i];
        hc[i] = p2[i];
        hd[i] = p3[i];
    }
    ko::deep_copy(a, ha);
    ko::deep_copy(b, hb);
    ko::deep_copy(c, hc);
    ko::deep_copy(d, hd);

    ko::View<Real, ExeSpace> res("res");
    ko::View<Real, ExeSpace>::HostMirror hres = ko::create_mirror_view(res);
    Vec<3, ExeSpace> vres("vres");
    Vec<3, ExeSpace>::HostMirror hvres = ko::create_mirror_view(vres);
    
    VecArr<3, ExeSpace> vecs("vecs", 4);
    
    ko::parallel_for(1, KOKKOS_LAMBDA (int i) {
        res(0) = SphereGeometry::triArea(a, b, c);
        if (std::abs(res(0) - PI/3.0) > ZERO_TOL) error("sphereTriArea error.");
        for (int j=0; j<3; ++j) {
            vecs(0, j) = a(j);
            vecs(1, j) = b(j);
            vecs(2, j) = c(j);
            vecs(3, j) = d(j);
        }
        SphereGeometry::midpoint(vres, slice(vecs, 0), slice(vecs,1));
        printf("sphere midpoint = (%f,%f,%f)\n", vres(0), vres(1), vres(2));
        if (std::abs(vres(0) - 0.5*std::sqrt(2.0)) > ZERO_TOL) error("sphereMidpoint error.");
        SphereGeometry::cross(vres, a, b);
        if (std::abs(vres(0)-2.0/3.0) > ZERO_TOL || 
            std::abs(vres(1)-2.0/3.0) > ZERO_TOL || 
            std::abs(vres(2)) > ZERO_TOL) error("cross product error.");
        res(0) = SphereGeometry::distance(a,b);
        printf("distance(a,b) = %f\n", res(0));
        SphereGeometry::barycenter(vres, vecs, 4);
        if (vres(0) != 1 || vres(1) != 0 || vres(2) != 0) error("sphere barycenter error.");
        res(0) = SphereGeometry::polygonArea(vres, vecs, 4);
        if (std::abs(res(0)-2*PI/3) > ZERO_TOL) error("sphere polygon error.");
    });
    
    
    } // END SPHERICAL TESTS
}
std::cout << "tests pass." << std::endl;
ko::finalize();
return 0;
}


