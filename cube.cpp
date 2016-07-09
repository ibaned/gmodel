#include "gmodel.hpp"

int main()
{
  auto c = gmod::new_cube(
      gmod::Vector{0,0,0},
      gmod::Vector{1,0,0},
      gmod::Vector{0,1,0},
      gmod::Vector{0,0,1});
  gmod::write_closure_to_geo(c, "cube.geo");
  gmod::write_closure_to_dmg(c, "cube.dmg");
}
