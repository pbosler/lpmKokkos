#include "LpmConfig.h"
#include "LpmDefs.hpp"

#include "LpmGeometry.hpp"
#include "LpmMeshSeed.hpp"
#include "LpmPolyMesh2d.hpp"
#include "LpmVtkIO.hpp"

#include "Kokkos_Core.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <exception>
#include <cassert>
#include <sstream>
#include <fstream>
#include <netcdf>

using namespace Lpm;

int main (int argc, char* argv[]) {
ko::initialize(argc, argv);
{

    std::cout << "Make spherical Voronoi mesh from icoshedral triangulation tests:\n";

}
ko::finalize();
return 0;
}
