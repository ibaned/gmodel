#include "gmodel.hpp"

int main()
{
  auto a = gmod::new_point2(gmod::Vector{0,0,0});
  auto b = gmod::new_point2(gmod::Vector{1,0,0});
  auto c = gmod::new_point2(gmod::Vector{1,1,0});
  auto d = gmod::new_point2(gmod::Vector{0.5,1.5,0});
  auto e = gmod::new_point2(gmod::Vector{0,1,0});
  auto l = gmod::new_loop();
  auto ab = gmod::new_line2(a,b);
  gmod::add_use(l, gmod::FORWARD, ab);
  auto bc = gmod::new_line2(b,c);
  gmod::add_use(l, gmod::FORWARD, bc);
  auto spline_pts = std::vector<gmod::PointPtr>({c,d,e});
  auto cde = gmod::new_spline2(spline_pts);
  gmod::add_use(l, gmod::FORWARD, cde);
  auto ea = gmod::new_line2(e,a);
  gmod::add_use(l, gmod::FORWARD, ea);
  auto f = gmod::new_plane2(l);
  auto r = gmod::extrude_face(f, gmod::Vector{0,0,1}).middle;
  gmod::write_closure_to_geo(r, "spline_shape.geo");
}

