#include <gmodel.hpp>
#include <minidiff.hpp>

int main()
{
  auto cube = gmod::new_cube(
      gmod::Vector{0,0,0},
      gmod::Vector{1,0,0},
      gmod::Vector{0,1,0},
      gmod::Vector{0,0,1});
  auto cube_bottom = gmod::get_cube_face(cube, gmod::BOTTOM);
  auto dimple_circle = gmod::new_circle(
      gmod::Vector{0.5,0.5,0},
      gmod::Vector{0,0,1},
      gmod::Vector{0.25,0,0});
  auto dimple_shell = gmod::new_shell();
  gmod::make_hemisphere(dimple_circle,
      new_point2(gmod::Vector{0.5,0.5,0}),
      dimple_shell, gmod::FORWARD);
  gmod::weld_half_shell_onto(cube, cube_bottom, dimple_shell, gmod::REVERSE);
  prevent_regression(cube, "dimple");
}

