#include "gmodel.hpp"

#include <map>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace gmod {

char const* const type_names[NTYPES] = {
    /* POINT   = */ "Point",
    /* LINE    = */ "Line",
    /* ARC     = */ "Circle",
    /* ELLIPSE = */ "Ellipse",
    /* SPLINE  = */ "Spline",
    /* PLANE   = */ "Plane Surface",
    /* RULED   = */ "Ruled Surface",
    /* VOLUME  = */ "Volume",
    /* LOOP    = */ "Line Loop",
    /* SHELL   = */ "Surface Loop",
    /* GROUP   = */ "Gmodel Group"};

char const* const physical_type_names[NTYPES] = {
    /* POINT   = */ "Physical Point",
    /* LINE    = */ "Physical Line",
    /* ARC     = */ "Physical Line",
    /* ELLIPSE = */ "Physical Line",
    /* SPLINE  = */ "Physical Line",
    /* PLANE   = */ "Physical Surface",
    /* RULED   = */ "Physical Surface",
    /* VOLUME  = */ "Physical Volume",
    /* LOOP    = */ 0,
    /* SHELL   = */ 0,
    /* GROUP   = */ 0};

int const type_dims[NTYPES] = {
    /* POINT   = */ 0,
    /* LINE    = */ 1,
    /* ARC     = */ 1,
    /* ELLIPSE = */ 1,
    /* SPLINE  = */ 1,
    /* PLANE   = */ 2,
    /* RULED   = */ 2,
    /* VOLUME  = */ 3,
    /* LOOP    = */ -1,
    /* SHELL   = */ -1,
    /* GROUP   = */ -1};

template <typename T>
static T& at(std::vector<T>& v, int i) {
  return v.at(std::size_t(i));
}

template <typename T>
static T const& at(std::vector<T> const& v, int i) {
  return v.at(std::size_t(i));
}

template <typename T>
static int size(std::vector<T> const& v) {
  return int(v.size());
}

int is_entity(int t) { return t <= VOLUME; }

int is_face(int t) { return t == PLANE || t == RULED; }

int is_boundary(int t) { return t == LOOP || t == SHELL; }

int get_boundary_type(int cell_type) {
  switch (type_dims[cell_type]) {
    case 3: return SHELL;
    case 2: return LOOP;
    default: return -1;
  }
}

static int next_id = 0;
static int nlive_objects = 0;

Object::Object(int type_) : type(type_), id(next_id++), scratch(-1) {
  ++nlive_objects;
}

ObjPtr new_object(int type) { return ObjPtr(new Object(type)); }

Object::~Object() { --nlive_objects; }

int get_used_dir(ObjPtr user, ObjPtr used) {
  auto it = std::find_if(user->used.begin(), user->used.end(),
                         [=](Use u) { return u.obj == used; });
  assert(it != user->used.end());
  return it->dir;
}

void print_object(FILE* f, ObjPtr obj) {
  switch (obj->type) {
    case POINT:
      print_point(f, std::dynamic_pointer_cast<Point>(obj));
      break;
    case ARC:
      print_arc(f, obj);
      break;
    case ELLIPSE:
      print_ellipse(f, obj);
      break;
    case SPLINE:
      print_spline(f, obj);
      break;
    case GROUP:
      break;
    default:
      print_simple_object(f, obj);
      break;
  }
}

void print_object_physical(FILE* f, ObjPtr obj) {
  if (!is_entity(obj->type)) return;
  fprintf(f, "%s(%u) = {%u};\n", physical_type_names[obj->type], obj->id,
          obj->id);
}

void print_closure(FILE* f, ObjPtr obj) {
  auto closure = get_closure(obj, 1);
  for (auto co : closure) print_object(f, co);
  closure = get_closure(obj, 0);
  for (auto co : closure) print_object_physical(f, co);
}

void write_closure_to_geo(ObjPtr obj, char const* filename) {
  FILE* f = fopen(filename, "w");
  print_closure(f, obj);
  fclose(f);
}

void print_simple_object(FILE* f, ObjPtr obj) {
  fprintf(f, "%s(%u) = {", type_names[obj->type], obj->id);
  bool first = true;
  for (auto use : obj->used) {
    if (!first) fprintf(f, ",");
    if (is_boundary(obj->type) && use.dir == REVERSE)
      fprintf(f, "%d", -(int(use.obj->id)));
    else
      fprintf(f, "%u", use.obj->id);
    if (first) first = false;
  }
  fprintf(f, "};\n");
}

void print_object_dmg(FILE* f, ObjPtr obj) {
  switch (obj->type) {
    case POINT: {
      PointPtr p = std::dynamic_pointer_cast<Point>(obj);
      fprintf(f, "%u %f %f %f\n", obj->id, p->pos.x, p->pos.y, p->pos.z);
    } break;
    case LINE:
    case ARC:
    case ELLIPSE: {
      fprintf(f, "%u %u %u\n", obj->id, edge_point(obj, 0)->id,
              edge_point(obj, 1)->id);
    } break;
    case PLANE:
    case RULED:
    case VOLUME: {
      fprintf(f, "%u %zu\n", obj->id, obj->used.size());
      for (auto use : obj->used) {
        auto bnd = use.obj;
        fprintf(f, " %zu\n", bnd->used.size());
        for (auto bu : bnd->used) {
          fprintf(f, "  %u %u\n", bu.obj->id, !bu.dir);
        }
      }
    } break;
    case LOOP:
    case SHELL:
    case GROUP:
      break;
  }
}

int count_of_type(std::vector<ObjPtr> const& objs, int type) {
  int c = 0;
  for (auto obj : objs)
    if (obj->type == type) ++c;
  return c;
}

int count_of_dim(std::vector<ObjPtr> const& objs, int dim) {
  int c = 0;
  for (int i = 0; i < NTYPES; ++i)
    if (is_entity(i) && type_dims[i] == dim) c += count_of_type(objs, i);
  return c;
}

void print_closure_dmg(FILE* f, ObjPtr obj) {
  auto closure = get_closure(obj, 0);
  fprintf(f, "%u %u %u %u\n", count_of_dim(closure, 3),
          count_of_dim(closure, 2), count_of_dim(closure, 1),
          count_of_dim(closure, 0));
  fprintf(f, "0 0 0\n0 0 0\n");
  for (auto co : closure) print_object_dmg(f, co);
}

void write_closure_to_dmg(ObjPtr obj, char const* filename) {
  FILE* f = fopen(filename, "w");
  print_closure_dmg(f, obj);
  fclose(f);
}

void add_use(ObjPtr by, int dir, ObjPtr of) {
  by->used.push_back(Use{dir, of});
}

void add_helper(ObjPtr to, ObjPtr h) { to->helpers.push_back(h); }

std::vector<ObjPtr> get_closure(ObjPtr obj, int include_helpers) {
  std::vector<ObjPtr> queue;
  queue.reserve(std::size_t(nlive_objects));
  std::size_t first = 0;
  queue.push_back(obj);
  while (first != queue.size()) {
    ObjPtr current = queue[first++];
    for (auto use : current->used) {
      auto child = use.obj;
      if (child->scratch == -1) {
        child->scratch = 1;
        queue.push_back(child);
      }
    }
    if (include_helpers)
      for (auto child : current->helpers) {
        if (child->scratch == -1) {
          child->scratch = 1;
          queue.push_back(child);
        }
      }
  }
  for (std::size_t i = 0; i < queue.size(); ++i) queue[i]->scratch = -1;
  std::reverse(queue.begin(), queue.end());
  return queue;
}

Point::Point() : Object(POINT) {}

Point::~Point() {}

PointPtr new_point() { return PointPtr(new Point()); }

double default_size = 0.1;

PointPtr new_point2(Vector v) {
  PointPtr p = new_point();
  p->pos = v;
  p->size = default_size;
  return p;
}

PointPtr new_point3(Vector v, double size) {
  PointPtr p = new_point();
  p->pos = v;
  p->size = size;
  return p;
}

std::vector<PointPtr> new_points(std::vector<Vector> vs) {
  std::vector<PointPtr> out;
  for (auto vec : vs) out.push_back(new_point2(vec));
  return out;
}

void print_point(FILE* f, PointPtr p) {
  fprintf(f, "Point(%u) = {%f,%f,%f,%f};\n", p->id, p->pos.x, p->pos.y,
          p->pos.z, p->size);
}

Extruded extrude_point(PointPtr start, Vector v) {
  PointPtr end = new_point3(add_vectors(start->pos, v), start->size);
  ObjPtr middle = new_line2(start, end);
  return Extruded{middle, end};
}

PointPtr edge_point(ObjPtr edge, int i) {
  auto o = edge->used[std::size_t(i)].obj;
  return std::dynamic_pointer_cast<Point>(o);
}

ObjPtr new_line() { return new_object(LINE); }

ObjPtr new_line2(PointPtr start, PointPtr end) {
  ObjPtr l = new_line();
  add_use(l, FORWARD, start);
  add_use(l, FORWARD, end);
  return l;
}

ObjPtr new_line3(Vector origin, Vector span) {
  return extrude_point(new_point2(origin), span).middle;
}

ObjPtr new_arc() { return new_object(ARC); }

ObjPtr new_arc2(PointPtr start, PointPtr center, PointPtr end) {
  ObjPtr a = new_arc();
  add_use(a, FORWARD, start);
  add_helper(a, center);
  add_use(a, FORWARD, end);
  return a;
}

PointPtr arc_center(ObjPtr arc) {
  return std::dynamic_pointer_cast<Point>(arc->helpers[0]);
}

Vector arc_normal(ObjPtr arc) {
  return normalize_vector(cross_product(
      subtract_vectors(edge_point(arc, 0)->pos, arc_center(arc)->pos),
      subtract_vectors(edge_point(arc, 1)->pos, arc_center(arc)->pos)));
}

void print_arc(FILE* f, ObjPtr arc) {
  fprintf(f, "%s(%u) = {%u,%u,%u};\n", type_names[arc->type], arc->id,
          edge_point(arc, 0)->id, arc_center(arc)->id, edge_point(arc, 1)->id);
}

ObjPtr new_ellipse() { return new_object(ELLIPSE); }

ObjPtr new_ellipse2(PointPtr start, PointPtr center, PointPtr major_pt,
                    PointPtr end) {
  ObjPtr e = new_ellipse();
  add_use(e, FORWARD, start);
  add_helper(e, center);
  add_helper(e, major_pt);
  add_use(e, FORWARD, end);
  return e;
}

PointPtr ellipse_center(ObjPtr e) {
  return std::dynamic_pointer_cast<Point>(e->helpers[0]);
}

PointPtr ellipse_major_pt(ObjPtr e) {
  return std::dynamic_pointer_cast<Point>(e->helpers[1]);
}

void print_ellipse(FILE* f, ObjPtr e) {
  fprintf(f, "%s(%u) = {%u,%u,%u,%u};\n", type_names[e->type], e->id,
          edge_point(e, 0)->id, ellipse_center(e)->id, ellipse_major_pt(e)->id,
          edge_point(e, 1)->id);
}

ObjPtr new_spline() { return new_object(SPLINE); }

ObjPtr new_spline2(std::vector<PointPtr> const& pts) {
  assert(pts.size() >= 2);
  auto e = new_spline();
  add_use(e, FORWARD, pts.front());
  for (size_t i = 1; i < pts.size() - 1; ++i) add_helper(e, pts[i]);
  add_use(e, FORWARD, pts.back());
  return e;
}

ObjPtr new_spline3(std::vector<Vector> const& pts) {
  std::vector<PointPtr> pts2;
  for (auto p : pts) pts2.push_back(new_point2(p));
  return new_spline2(pts2);
}

void print_spline(FILE* f, ObjPtr e) {
  fprintf(f, "%s(%u) = {%u,", type_names[e->type], e->id, edge_point(e, 0)->id);
  for (auto h : e->helpers) fprintf(f, "%u,", h->id);
  fprintf(f, "%u};\n", edge_point(e, 1)->id);
}

Extruded extrude_edge(ObjPtr start, Vector v) {
  Extruded left = extrude_point(edge_point(start, 0), v);
  Extruded right = extrude_point(edge_point(start, 1), v);
  return extrude_edge2(start, v, left, right);
}

Extruded extrude_edge2(ObjPtr start, Vector v, Extruded left, Extruded right) {
  auto loop = new_loop();
  add_use(loop, FORWARD, start);
  add_use(loop, FORWARD, right.middle);
  ObjPtr end = 0;
  switch (start->type) {
    case LINE: {
      end = new_line2(std::dynamic_pointer_cast<Point>(left.end),
                      std::dynamic_pointer_cast<Point>(right.end));
      break;
    }
    case ARC: {
      PointPtr start_center = arc_center(start);
      PointPtr end_center =
          new_point3(add_vectors(start_center->pos, v), start_center->size);
      end = new_arc2(std::dynamic_pointer_cast<Point>(left.end), end_center,
                     std::dynamic_pointer_cast<Point>(right.end));
      break;
    }
    case ELLIPSE: {
      PointPtr start_center = ellipse_center(start);
      PointPtr end_center =
          new_point3(add_vectors(start_center->pos, v), start_center->size);
      PointPtr start_major_pt = ellipse_major_pt(start);
      PointPtr end_major_pt =
          new_point3(add_vectors(start_major_pt->pos, v), start_major_pt->size);
      end = new_ellipse2(std::dynamic_pointer_cast<Point>(left.end), end_center,
                         end_major_pt,
                         std::dynamic_pointer_cast<Point>(right.end));
      break;
    }
    case SPLINE: {
      std::vector<PointPtr> end_pts;
      end_pts.push_back(std::dynamic_pointer_cast<Point>(left.end));
      for (auto h : start->helpers) {
        auto start_h = std::dynamic_pointer_cast<Point>(h);
        auto end_h = new_point3(add_vectors(start_h->pos, v), start_h->size);
        end_pts.push_back(end_h);
      }
      end_pts.push_back(std::dynamic_pointer_cast<Point>(right.end));
      end = new_spline2(end_pts);
      break;
    }
    default:
      end = 0;
      break;
  }
  add_use(loop, REVERSE, end);
  add_use(loop, REVERSE, left.middle);
  ObjPtr middle;
  switch (start->type) {
    case LINE:
      middle = new_plane2(loop);
      break;
    case ARC:
    case ELLIPSE:
    case SPLINE:
      middle = new_ruled2(loop);
      break;
    default:
      middle = 0;
      break;
  }
  return Extruded{middle, end};
}

ObjPtr new_loop() { return new_object(LOOP); }

std::vector<PointPtr> loop_points(ObjPtr loop) {
  std::vector<PointPtr> points;
  for (auto use : loop->used) points.push_back(edge_point(use.obj, use.dir));
  return points;
}

Extruded extrude_loop(ObjPtr start, Vector v) {
  ObjPtr shell = new_shell();
  return extrude_loop2(start, v, shell, FORWARD);
}

Extruded extrude_loop2(ObjPtr start, Vector v, ObjPtr shell, int shell_dir) {
  ObjPtr end = new_loop();
  std::vector<PointPtr> start_points = loop_points(start);
  int n = size(start_points);
  std::vector<Extruded> point_extrusions;
  for (auto start_point : start_points)
    point_extrusions.push_back(extrude_point(start_point, v));
  std::vector<Extruded> edge_extrusions;
  for (int i = 0; i < n; ++i) {
    auto use = at(start->used, i);
    edge_extrusions.push_back(
        extrude_edge2(use.obj, v, at(point_extrusions, (i + (use.dir ^ 0)) % n),
                      at(point_extrusions, (i + (use.dir ^ 1)) % n)));
  }
  for (int i = 0; i < n; ++i)
    add_use(end, at(start->used, i).dir, at(edge_extrusions, i).end);
  for (int i = 0; i < n; ++i)
    add_use(shell, at(start->used, i).dir ^ shell_dir,
            at(edge_extrusions, i).middle);
  return Extruded{shell, end};
}

ObjPtr new_circle(Vector center, Vector normal, Vector x) {
  Matrix r = rotation_matrix(normal, PI / 2);
  PointPtr center_point = new_point2(center);
  PointPtr ring_points[4];
  for (int i = 0; i < 4; ++i) {
    ring_points[i] = new_point2(add_vectors(center, x));
    x = matrix_vector_product(r, x);
  }
  ObjPtr loop = new_loop();
  for (int i = 0; i < 4; ++i) {
    ObjPtr a = new_arc2(ring_points[i], center_point, ring_points[(i + 1) % 4]);
    add_use(loop, FORWARD, a);
  }
  return loop;
}

ObjPtr new_ellipse3(Vector center, Vector major, Vector minor) {
  auto center_point = new_point2(center);
  PointPtr ring_points[4];
  ring_points[0] = new_point2(center + major);
  ring_points[1] = new_point2(center + minor);
  ring_points[2] = new_point2(center - major);
  ring_points[3] = new_point2(center - minor);
  auto major_point = new_point2(center + (major / 2));
  auto loop = new_loop();
  for (int i = 0; i < 4; ++i) {
    auto a = new_ellipse2(ring_points[i], center_point, major_point,
                          ring_points[(i + 1) % 4]);
    add_use(loop, FORWARD, a);
  }
  return loop;
}

ObjPtr new_polyline(std::vector<PointPtr> const& pts) {
  auto loop = new_loop();
  for (std::size_t i = 0; i < pts.size(); ++i) {
    auto line = new_line2(pts[i], pts[(i + 1) % pts.size()]);
    add_use(loop, FORWARD, line);
  }
  return loop;
}

ObjPtr new_polyline2(std::vector<Vector> const& vs) {
  return new_polyline(new_points(vs));
}

ObjPtr new_plane() { return new_object(PLANE); }

ObjPtr new_plane2(ObjPtr loop) {
  ObjPtr p = new_plane();
  add_use(p, FORWARD, loop);
  return p;
}

ObjPtr new_square(Vector origin, Vector x, Vector y) {
  return extrude_edge(new_line3(origin, x), y).middle;
}

ObjPtr new_disk(Vector center, Vector normal, Vector x) {
  return new_plane2(new_circle(center, normal, x));
}

ObjPtr new_elliptical_disk(Vector center, Vector major, Vector minor) {
  return new_plane2(new_ellipse3(center, major, minor));
}

ObjPtr new_polygon(std::vector<Vector> const& vs) {
  return new_plane2(new_polyline2(vs));
}

ObjPtr new_ruled() { return new_object(RULED); }

ObjPtr new_ruled2(ObjPtr loop) {
  ObjPtr p = new_ruled();
  add_use(p, FORWARD, loop);
  return p;
}

void add_hole_to_face(ObjPtr face, ObjPtr loop) {
  add_use(face, REVERSE, loop);
}

Extruded extrude_face(ObjPtr face, Vector v) {
  assert(type_dims[face->type] == 2);
  ObjPtr end;
  switch (face->type) {
    case PLANE:
      end = new_plane();
      break;
    case RULED:
      end = new_ruled();
      break;
    default:
      end = nullptr;
      break;
  }
  auto shell = new_shell();
  add_use(shell, REVERSE, face);
  add_use(shell, FORWARD, end);
  for (auto use : face->used) {
    auto end_loop = extrude_loop2(use.obj, v, shell, use.dir).end;
    add_use(end, use.dir, end_loop);
  }
  auto middle = new_volume2(shell);
  return Extruded{middle, end};
}

ObjPtr face_loop(ObjPtr face) { return face->used[0].obj; }

ObjPtr new_shell() { return new_object(SHELL); }

void make_hemisphere(ObjPtr circle, PointPtr center, ObjPtr shell, int dir) {
  assert(circle->used.size() == 4);
  Vector normal = arc_normal(circle->used[0].obj);
  if (dir == REVERSE) normal = scale_vector(-1, normal);
  auto circle_points = loop_points(circle);
  double radius =
      vector_norm(subtract_vectors(circle_points[0]->pos, center->pos));
  Vector cap_pos = add_vectors(center->pos, scale_vector(radius, normal));
  PointPtr cap = new_point2(cap_pos);
  ObjPtr inward[4];
  for (std::size_t i = 0; i < 4; ++i)
    inward[i] = new_arc2(circle_points[i], center, cap);
  for (std::size_t i = 0; i < 4; ++i) {
    auto loop = new_loop();
    add_use(loop, circle->used[i].dir ^ dir, circle->used[i].obj);
    add_use(loop, FORWARD ^ dir, inward[(i + 1) % 4]);
    add_use(loop, REVERSE ^ dir, inward[i]);
    add_use(shell, FORWARD, new_ruled2(loop));
  }
}

ObjPtr new_sphere(Vector center, Vector normal, Vector x) {
  ObjPtr circle = new_circle(center, normal, x);
  PointPtr cpt = arc_center(circle->used[0].obj);
  ObjPtr shell = new_shell();
  make_hemisphere(circle, cpt, shell, FORWARD);
  make_hemisphere(circle, cpt, shell, REVERSE);
  return shell;
}

ObjPtr new_volume() { return new_object(VOLUME); }

ObjPtr new_volume2(ObjPtr shell) {
  ObjPtr v = new_object(VOLUME);
  add_use(v, FORWARD, shell);
  return v;
}

ObjPtr volume_shell(ObjPtr v) { return v->used[0].obj; }

ObjPtr new_cube(Vector origin, Vector x, Vector y, Vector z) {
  return extrude_face(new_square(origin, x, y), z).middle;
}

ObjPtr get_cube_face(ObjPtr cube, enum cube_face which) {
  return cube->used[0].obj->used[which].obj;
}

ObjPtr new_ball(Vector center, Vector normal, Vector x) {
  return new_volume2(new_sphere(center, normal, x));
}

void insert_into(ObjPtr into, ObjPtr o) {
  if (is_face(o->type)) {
    assert(is_face(into->type));
    add_use(into, REVERSE, face_loop(o));
  } else if (o->type == VOLUME) {
    assert(into->type == VOLUME);
    add_use(into, REVERSE, volume_shell(o));
  } else {
    fprintf(stderr, "unexpected inserted type \"%s\"\n",
        type_names[o->type]);
    abort();
  }
}

ObjPtr new_group() { return new_object(GROUP); }

void add_to_group(ObjPtr group, ObjPtr o) { add_use(group, FORWARD, o); }

void weld_volume_face_into(ObjPtr big_volume, ObjPtr small_volume,
                           ObjPtr big_volume_face, ObjPtr small_volume_face) {
  insert_into(big_volume_face, small_volume_face);
  add_use(volume_shell(big_volume),
          !(get_used_dir(volume_shell(small_volume), small_volume_face)),
          small_volume_face);
}

static int are_parallel(Vector a, Vector b) {
  return 1e-6 >
         (1.0 - fabs(dot_product(normalize_vector(a), normalize_vector(b))));
}

static int are_perpendicular(Vector a, Vector b) {
  return 1e-6 >
         (0.0 - fabs(dot_product(normalize_vector(a), normalize_vector(b))));
}

Vector eval(ObjPtr o, double const* param) {
  switch (o->type) {
    case POINT: {
      auto p = std::dynamic_pointer_cast<Point>(o);
      return p->pos;
    }
    case LINE: {
      double u = param[0];
      PointPtr a = edge_point(o, 0);
      PointPtr b = edge_point(o, 1);
      return add_vectors(scale_vector(1.0 - u, a->pos),
                         scale_vector(u, b->pos));
    }
    case ARC: {
      double u = param[0];
      PointPtr a = edge_point(o, 0);
      PointPtr c = arc_center(o);
      PointPtr b = edge_point(o, 1);
      Vector n = arc_normal(o);
      Vector ca = subtract_vectors(a->pos, c->pos);
      Vector cb = subtract_vectors(b->pos, c->pos);
      double full_ang =
          acos(dot_product(ca, cb) / (vector_norm(ca) * vector_norm(cb)));
      double ang = full_ang * u;
      return rotate_vector(n, ang, ca);
    }
    case ELLIPSE: {
      double u = param[0];
      PointPtr a = edge_point(o, 0);
      PointPtr c = ellipse_center(o);
      PointPtr m = ellipse_major_pt(o);
      PointPtr b = edge_point(o, 1);
      Vector ca = subtract_vectors(a->pos, c->pos);
      Vector cb = subtract_vectors(b->pos, c->pos);
      Vector cm = subtract_vectors(m->pos, c->pos);
      if (!are_parallel(cb, cm)) {
        PointPtr tmp = a;
        a = b;
        b = tmp;
        u = 1.0 - u;
        if (!are_parallel(cb, cm)) {
          fprintf(stderr, "gmodel only understands quarter ellipses,\n");
          fprintf(stderr, "and this one has no endpoint on the major axis\n");
          abort();
        }
        if (!are_perpendicular(ca, cm)) {
          fprintf(stderr, "gmodel only understands quarter ellipses,\n");
          fprintf(stderr, "and this one has no endpoint on the minor axis\n");
          abort();
        }
      }
      double full_ang = PI / 2.0;
      double ang = full_ang * u;
      return add_vectors(c->pos, add_vectors(scale_vector(cos(ang), ca),
                                             scale_vector(sin(ang), cb)));
    }
    default:
      return Vector{-42, -42, -42};
  }
}

void transform_closure(ObjPtr object, Matrix linear, Vector translation) {
  auto closure = get_closure(object, 1);
  for (auto co : closure) {
    if (co->type == POINT) {
      auto pt = std::dynamic_pointer_cast<Point>(co);
      pt->pos = (linear * (pt->pos)) + translation;
    }
  }
}

static ObjPtr copy_object(ObjPtr object) {
  ObjPtr out;
  if (object->type == POINT) {
    auto point = std::dynamic_pointer_cast<Point>(object);
    out = new_point3(point->pos, point->size);
  } else {
    out = new_object(object->type);
  }
  return out;
}

ObjPtr copy_closure(ObjPtr object) {
  auto closure = get_closure(object, 1);
  for (size_t i = 0; i < closure.size(); ++i) closure[i]->scratch = i;
  decltype(closure) out_closure;
  for (auto co : closure) {
    auto oco = copy_object(co);
    for (auto coh : co->helpers) {
      auto idx = coh->scratch;
      assert(idx < co->scratch);
      oco->helpers.push_back(at(out_closure, idx));
    }
    for (auto cou : co->used) {
      auto idx = cou.obj->scratch;
      assert(idx < co->scratch);
      add_use(oco, cou.dir, at(out_closure, idx));
    }
    out_closure.push_back(oco);
  }
  for (auto co : closure) co->scratch = -1;
  return out_closure.back();
}

ObjPtr collect_assembly_boundary(ObjPtr assembly) {
  std::vector<Use> uses;
  int cell_type = -1;
  for (auto cell_use : assembly->used) {
    auto cell = cell_use.obj;
    if (cell_type == -1)
      cell_type = cell->type;
    assert(cell_type == cell->type);
    auto boundary = cell->used[0].obj;
    for (auto side_use : boundary->used) {
      uses.push_back(side_use);
    }
  }
  for (auto use : uses)
    use.obj->scratch = 0;
  for (auto use : uses)
    ++(use.obj->scratch);
  auto boundary = new_object(get_boundary_type(cell_type));
  for (auto use : uses)
    if (use.obj->scratch == 1)
      boundary->used.push_back(use);
  for (auto use : uses)
    use.obj->scratch = -1;
  return boundary;
}

}  // end namespace gmod
