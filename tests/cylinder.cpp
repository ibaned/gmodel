#include <gmodel.hpp>
#include <minidiff.hpp>

int main()
{
  auto d = gmod::new_disk(gmod::Vector{0,0,0},
                          gmod::Vector{0,0,1},
                          gmod::Vector{1,0,0});
  auto c = gmod::extrude_face(d, gmod::Vector{0,0,1}).middle;
  prevent_regression(c, "cylinder");
}
