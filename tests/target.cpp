#include <gmodel.hpp>
#include <minidiff.hpp>

int main()
{
  auto outer_face = gmod::new_disk(
      gmod::Vector{0,0,0},
      gmod::Vector{0,0,1},
      gmod::Vector{2,0,0});
  auto inner_face = gmod::new_disk(
      gmod::Vector{0,0,0},
      gmod::Vector{0,0,1},
      gmod::Vector{1,0,0});
  gmod::insert_into(outer_face, inner_face);
  auto face_group = gmod::new_group();
  gmod::add_to_group(face_group, inner_face);
  gmod::add_to_group(face_group, outer_face);
  auto ext = gmod::extrude_face_group(face_group,
      [](gmod::Vector a){return a + gmod::Vector{0,0,0.2};});
  auto volume_group = ext.middle;
  prevent_regression(volume_group, "target");
}

