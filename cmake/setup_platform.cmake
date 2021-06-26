
macro(setup_platform)

  if (UNIX AND NOT APPLE)
    set(LINUX ON)
  endif()

  # do we have openmp?
  find_package(OpenMP)
  if (OpenMP-NOTFOUND)
    set(LPM_USE_OPENMP FALSE)
  else()
    set(LPM_USE_OPENMP TRUE)
  endif()
  if (APPLE)
    set(LPM_USE_OPENMP FALSE)
  endif()

  # Do we have bash?
  find_program(BASH bash)
  if (BASH STREQUAL "BASH_NOTFOUND")
    message(FATAL_ERROR "Bash is required, but is not available on this system.")
  endif()

  # Do we have make?
  find_program(MAKE make)
  if (MAKE STREQUAL "MAKE_NOTFOUND")
    message(FATAL_ERROR "Make is required, but is not available on this system.")
  endif()

  # Do we have git?
  find_program(GIT git)
  if (GIT STREQUAL "GIT_NOTFOUND")
    message(WARNING "Git not found. Hope you're not developing on this system.")
    set(HAVE_GIT FALSE)
  else()
    set(HAVE_GIT TRUE)
  endif()

  # Do we have graphics (OpenGL)?
  find_package(OpenGL QUIET)
  if (OPENGL_FOUND)
    set(LPM_USE_VTK_GRAPHICS TRUE)
  else()
    set(LPM_USE_VTK_GRAPHICS FALSE)
  endif()

  if (DEVICE STREQUAL "CUDA")
    set(LPM_USE_CUDA TRUE)
  else()
    set(LPM_USE_CUDA FALSE)
  endif()

  include(GNUInstallDirs)

  if (HDF5_INCLUDE_DIR)
    if (NOT EXISTS ${HDF5_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find HDF5 include dir at ${HDF5_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using HDF5 include dir: ${HDF5_INCLUDE_DIR}.")
  else()
    set(HDF5_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()

  if (HDF5_LIBRARY)
    if (NOT EXISTS ${HDF5_LIBRARY})
      message(FATAL_ERROR "Couldn't find HDF5 library at ${HDF5_LIBRARY}.")
    endif()
    message(STATUS "Using HDF5 library at ${HDF5_LIBRARY}.")
  else()
    set(HDF5_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(HDF5_LIBRARY "${HDF5_LIBRARY_DIR}/libhdf5.a")
    message(STATUS "Building HDF5 library: ${HDF5_LIBRARY}.")
  endif()

  get_filename_component(HDF5_LIBRARY_DIR ${HDF5_LIBRARY} DIRECTORY)
  if (HDF5_HL_LIBRARY)
    if (NOT EXISTS ${HDF5_HL_LIBRARY})
      message(FATAL_ERROR "Couldn't find high-level HDF5 library at ${HDF5_HL_LIBRARY}.")
    endif()
    message(STATUS "Using high-level HDF5 library at ${HDF5_HL_LIBRARY}.")
  else()
    set(HDF5_HL_LIBRARY "${HDF5_LIBRARY_DIR}/libhdf5_hl.a")
    message(STATUS "Building high-level HDF5 library: ${HDF5_HL_LIBRARY}.")
  endif()

  if (NETCDF_INCLUDE_DIR)
    if (NOT EXISTS ${NETCDF_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find NetCDF include dir at ${NETCDF_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using NetCDF include dir: ${NETCDF_INCLUDE_DIR}.")
  else()
    set(NETCDF_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (NETCDF_LIBRARY)
    if (NOT EXISTS ${NETCDF_LIBRARY})
      message(FATAL_ERROR "Couldn't find NetCDF library at ${NETCDF_LIBRARY}.")
    endif()
    message(STATUS "Using NetCDF library at ${NETCDF_LIBRARY}.")
  else()
    set(NETCDF_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(NETCDF_LIBRARY "${NETCDF_LIBRARY_DIR}/libnetcdf.a")
    message(STATUS "Building NetCDF library: ${NETCDF_LIBRARY}.")
  endif()
  get_filename_component(NETCDF_LIBRARY_DIR ${NETCDF_LIBRARY} DIRECTORY)

  if (YAMLCPP_INCLUDE_DIR)
    if (NOT EXISTS ${YAMLCPP_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find yaml-cpp include dir at ${YAMLCPP_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using yaml-cpp include dir: ${YAMLCPP_INCLUDE_DIR}.")
  else()
    set(YAMLCPP_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (YAMLCPP_LIBRARY)
    if (NOT EXISTS ${YAMLCPP_LIBRARY})
      message(FATAL_ERROR "Couldn't find yaml-cpp library at ${YAMLCPP_LIBRARY}.")
    endif()
    message(STATUS "Using yaml-cpp library at ${YAMLCPP_LIBRARY}.")
  else()
    set(YAMLCPP_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(YAMLCPP_LIBRARY ${YAMLCPP_LIBRARY_DIR}/libyaml-cpp.a)
    message(STATUS "Building yaml-cpp library: ${YAMLCPP_LIBRARY}.")
  endif()
  get_filename_component(YAMLCPP_LIBRARY_DIR ${YAMLCPP_LIBRARY} DIRECTORY)

  if (TRILINOS_INCLUDE_DIR)
    if (NOT EXISTS ${TRILINOS_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find trilinos include dir at ${TRILINOS_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using trilinos include dir: ${TRILINOS_INCLUDE_DIR}.")
    if (NOT EXISTS ${TRILINOS_TPL_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find trilinos tpl include dir at ${TRILINOS_TPL_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using trilinos tpl include dir: ${TRILINOS_TPL_INCLUDE_DIR}.")
  else()
    set(TRILINOS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (TRILINOS_LIBRARY_DIR)
    if (NOT EXISTS ${TRILINOS_LIBRARY_DIR})
      message(FATAL_ERROR "Couldn't find trilinos library dir: ${TRILINOS_LIBRARY_DIR}.")
    endif()
    message(STATUS "Using trilinos library dir: ${TRILINOS_LIBRARY_DIR}.")
    if (NOT EXISTS ${TRILINOS_TPL_LIBRARY_DIR})
      message(FATAL_ERROR "Couldn't find trilinos tpl library dir: ${TRILINOS_TPL_LIBRARY_DIR}.")
    endif()
    message(STATUS "Using trilinos tpl library dir: ${TRILINOS_TPL_LIBRARY_DIR}.")
  else()
    set(TRILINOS_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(TRILINOS_TPL_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "Building trilinos libraries in: ${TRILINOS_LIBRARY_DIR}.")
  endif()

  if (VTK_INCLUDE_DIR)
    if (NOT EXISTS ${VTK_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find vtk include dir: ${VTK_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using vtk include dir: ${VTK_INCLUDE_DIR}.")
  else()
    set(VTK_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (VTK_LIBRARY_DIR)
    if (NOT EXISTS ${VTK_LIBRARY_DIR})
      message(FATAL_ERROR "Couldn't find vtk library dir at: ${VTK_LIBRARY_DIR}.")
    endif()
    message(STATUS "Using vtk include dir: ${VTK_INCLUDE_DIR}.")
  else()
    set(VTK_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "Building vtk libraries in: ${VTK_LIBRARY_DIR}.")
  endif()

  # Certain tools (e.g. patch) require TMPDIR to be defined. If it is not,
  # we do so here.
  set(TMPDIR_VAR $ENV{TMPDIR})
  if (NOT TMPDIR_VAR)
    set(ENV{TMPDIR} "/tmp")
  endif()
endmacro()
