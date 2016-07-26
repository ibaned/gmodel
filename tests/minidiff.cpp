#include "minidiff.hpp"

#include <fstream>
#include <cassert>

static bool are_same(std::string const& path1, std::string const& path2) {
  std::ifstream file1(path1.c_str());
  std::ifstream file2(path2.c_str());
  assert(file1.is_open());
  assert(file2.is_open());
  char c1, c2;
  while (file1.get(c1)) {
    if (!file2.get(c2)) return false;
    if (c1 != c2) return false;
  }
  return true;
}

void prevent_regression(gmod::ObjPtr model, std::string const& name) {
  std::string gold_name = name + "_gold";
  std::string geo_name = name + ".geo";
  std::string dmg_name = name + ".dmg";
  std::string gold_geo_name = gold_name + ".geo";
  std::string gold_dmg_name = gold_name + ".dmg";
  gmod::write_closure_to_geo(model, geo_name.c_str());
  gmod::write_closure_to_dmg(model, dmg_name.c_str());
  assert(are_same(geo_name, gold_geo_name));
  assert(are_same(dmg_name, gold_dmg_name));
}
