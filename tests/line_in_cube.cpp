#include <gmodel.hpp>
#include <minidiff.hpp>

int main()
{
  gmod::default_size = 0.5;
  auto c = gmod::new_cube(
      gmod::Vector{0,0,0},
      gmod::Vector{1,0,0},
      gmod::Vector{0,1,0},
      gmod::Vector{0,0,1});
  gmod::default_size = 0.05;
  auto l = gmod::new_line4(gmod::Vector{.25,.5,.5}, gmod::Vector{.75,.5,.5});
  gmod::embed(c, l);
  prevent_regression(c, "line_in_cube");
}

