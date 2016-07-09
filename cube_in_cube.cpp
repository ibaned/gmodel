#include "gmodel.hpp"

int main()
{
  auto outer = gmod::new_cube(
      gmod::Vector{0,0,0},
      gmod::Vector{1,0,0},
      gmod::Vector{0,1,0},
      gmod::Vector{0,0,1});
  auto inner = gmod::new_cube(
      gmod::Vector{1./3.,1./3.,1./3.},
      gmod::Vector{1./3.,0,0},
      gmod::Vector{0,1./3.,0},
      gmod::Vector{0,0,1./3.});
  gmod::insert_into(outer, inner);
  gmod::write_closure_to_geo(outer, "cube_in_cube.geo");
}
