#include <gmodel.hpp>
#include <minidiff.hpp>
#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <float.h>

bool readFileCoords(std::string filePath, std::vector<double>& buffer);

bool readFileCoords(std::string filePath, std::vector<double>& buffer) {
    std::ifstream file(filePath, std::ios::binary);
    if (file.fail()) {
      perror(filePath.c_str());
      return false;
    }
    for(std::string line; std::getline(file, line); ) {
      if(line.at(0) == '%')
        continue;
      std::istringstream in(line);
      std::string type;
      double x, y;
      in >> x >> y;
      buffer.push_back(x);
      buffer.push_back(y);
    }
    file.close();
    return true;
}

int main(int argc, char** argv)
{
  assert(argc==3);
  std::string in = std::string(argv[1]);
  std::string geo = std::string(argv[2]) + ".geo";
  std::string dmg = std::string(argv[2]) + ".dmg";

  std::vector<double> xy;
  bool read = readFileCoords(in, xy);
  assert(read);

  // gather the points with +y, top, and -y, bottom, coords
  // skip the coords with y=0, we'll add those later
  auto spline_pts_top = std::vector<gmod::PointPtr>();
  auto spline_pts_bot = std::vector<gmod::PointPtr>();
  auto spline_pts_zero = std::vector<gmod::PointPtr>();
  for(size_t i=0; i<xy.size(); i+=2) {
    auto a = gmod::new_point2(gmod::Vector{xy[i],xy[i+1],0});
    if( xy[i+1] > 0 )
      spline_pts_top.push_back(a);
    if( xy[i+1] < 0 )
      spline_pts_bot.push_back(a);
    else
      spline_pts_zero.push_back(a);
  }

  gmod::PointPtr leading;
  gmod::PointPtr trailing;
  leading = spline_pts_zero.front();
  trailing = spline_pts_zero.front();
  for( auto& pt : spline_pts_zero ) {
    if( pt->pos.x > trailing->pos.x )
      trailing = pt;
    if( pt->pos.x < leading->pos.x )
      leading = pt;
  }
  printf("leading (%f,%d) trailing (%f,%d)\n",
      leading->pos.x, leading->id,
      trailing->pos.x, trailing->id);
  spline_pts_top.insert(spline_pts_top.begin(),trailing);
  spline_pts_top.push_back(leading);
  spline_pts_bot.insert(spline_pts_bot.begin(),leading);
  spline_pts_bot.push_back(trailing);

  auto l = gmod::new_loop();
  auto spline_bot = gmod::new_spline2(spline_pts_bot);
  gmod::add_use(l, gmod::FORWARD, spline_bot);
  auto spline_top = gmod::new_spline2(spline_pts_top);
  gmod::add_use(l, gmod::FORWARD, spline_top);

  auto origin = gmod::Vector{-.5,-.5,0};
  auto xlim = gmod::Vector{2,0,0};
  auto ylim = gmod::Vector{0,1,0};
  auto sq = gmod::new_square(origin,xlim,ylim);

  gmod::add_hole_to_face(sq,l);
  auto r = gmod::extrude_face(sq, gmod::Vector{0,0,.3}).middle;

  write_closure_to_geo(r,geo.c_str());
  write_closure_to_dmg(r,dmg.c_str());
  return 0;
}
