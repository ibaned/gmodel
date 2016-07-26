#include <gmodel.hpp>
#include <minidiff.hpp>

int main()
{
  auto c = gmod::new_cube(
      gmod::Vector{0,0,0},
      gmod::Vector{1,0,0},
      gmod::Vector{0,1,0},
      gmod::Vector{0,0,1});
  prevent_regression(c, "cube");
}
