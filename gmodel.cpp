#include "gmodel.hpp"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

namespace gmod {

char const* const type_names[NTYPES] = {
  [POINT]   = "Point",
  [LINE]    = "Line",
  [ARC]     = "Circle",
  [ELLIPSE] = "Ellipse",
  [PLANE]   = "Plane Surface",
  [RULED]   = "Ruled Surface",
  [VOLUME]  = "Volume",
  [LOOP]    = "Line Loop",
  [SHELL]   = "Surface Loop",
  [GROUP]   = 0
};

char const* const physical_type_names[NTYPES] = {
  [POINT]   = "Physical Point",
  [LINE]    = "Physical Line",
  [ARC]     = "Physical Line",
  [ELLIPSE] = "Physical Line",
  [PLANE]   = "Physical Surface",
  [RULED]   = "Physical Surface",
  [VOLUME]  = "Physical Volume",
  [LOOP]    = 0,
  [SHELL]   = 0,
  [GROUP]   = 0
};

unsigned const type_dims[NTYPES] = {
  [POINT]   = 0,
  [LINE]    = 1,
  [ARC]     = 1,
  [ELLIPSE] = 1,
  [PLANE]   = 2,
  [RULED]   = 2,
  [VOLUME]  = 3,
  [LOOP]    = 0,
  [SHELL]   = 0,
  [GROUP]   = 0
};

unsigned is_entity(Type t)
{
  return t <= VOLUME;
}

unsigned is_boundary(Type t)
{
  return t == LOOP || t == SHELL;
}

static unsigned next_id = 0;
static unsigned nlive_objects = 0;

void init_object(Object* obj, Type type, dtor_t dtor)
{
  obj->type = type;
  obj->id = next_id++;
  obj->ref_count = 0;
  obj->dtor = dtor;
  obj->visited = 0;
  ++nlive_objects;
}

Object* new_object(Type type, dtor_t dtor)
{
  Object* o = new Object;
  init_object(o, type, dtor);
  return o;
}

void free_object(Object* obj)
{
  for (auto use : obj->used)
    drop_object(use.obj);
  for (auto helper : obj->helpers)
    drop_object(helper);
  delete obj;
  --nlive_objects;
}

void grab_object(Object* obj)
{
  ++obj->ref_count;
}

void drop_object(Object* obj)
{
  assert(obj->ref_count);
  --obj->ref_count;
  if (!obj->ref_count)
    obj->dtor(obj);
}

UseDir get_used_dir(Object* user, Object* used)
{
  auto it = std::find_if(user->used.begin(), user->used.end(),
      [=](Use u){return u.obj == used;});
  assert(it != user->used.end());
  return it->dir;
}

void print_object(FILE* f, Object* obj)
{
  switch (obj->type) {
    case POINT: print_point(f, (Point*)obj); break;
    case ARC: print_arc(f, obj); break;
    case ELLIPSE: print_ellipse(f, obj); break;
    case GROUP: break;
    default: print_simple_object(f, obj); break;
  }
}

void print_object_physical(FILE* f, Object* obj)
{
  if (!is_entity(obj->type))
    return;
  fprintf(f, "%s(%u) = {%u};\n", physical_type_names[obj->type],
      obj->id, obj->id);
}

void print_closure(FILE* f, Object* obj)
{
  auto closure = get_closure(obj, 1);
  for (auto it = closure.rbegin(); it != closure.rend(); ++it)
    print_object(f, *it);
  closure = get_closure(obj, 0);
  for (auto it = closure.rbegin(); it != closure.rend(); ++it)
    print_object_physical(f, *it);
}

void write_closure_to_geo(Object* obj, char const* filename)
{
  FILE* f = fopen(filename, "w");
  print_closure(f, obj);
  fclose(f);
}

void print_simple_object(FILE* f, Object* obj)
{
  fprintf(f, "%s(%u) = {", type_names[obj->type], obj->id);
  bool first = false;
  for (auto use : obj->used) {
    if (!first) fprintf(f, ",");
    if (is_boundary(obj->type) && use.dir == REVERSE)
      fprintf(f, "%d", -((int)(use.obj->id)));
    else
      fprintf(f, "%u", use.obj->id);
    if (first) first = false;
  }
  fprintf(f, "};\n");
}

void print_object_dmg(FILE* f, Object* obj)
{
  switch (obj->type) {
    case POINT: {
      Point* p = (Point*) obj;
      fprintf(f, "%u %f %f %f\n", obj->id,
          p->pos.x, p->pos.y, p->pos.z);
    } break;
    case LINE:
    case ARC:
    case ELLIPSE: {
      fprintf(f, "%u %u %u\n", obj->id,
          edge_point(obj, 0)->obj.id,
          edge_point(obj, 1)->obj.id);
    } break;
    case PLANE:
    case RULED:
    case VOLUME: {
      fprintf(f, "%u %zu\n", obj->id, obj->used.size());
      for (auto use : obj->used) {
        print_object_dmg(f, use.obj);
      }
    } break;
    case LOOP:
    case SHELL: {
      fprintf(f, " %zu\n", obj->used.size());
      for (auto u : obj->used) {
        fprintf(f, "  %u %u\n", u.obj->id, !u.dir);
      }
    } break;
    case GROUP: {
    } break;
  }
}

unsigned count_of_type(std::vector<Object*> const& objs, Type type)
{
  unsigned c = 0;
  for (auto obj : objs)
    if (obj->type == type)
      ++c;
  return c;
}

unsigned count_of_dim(std::vector<Object*> const& objs, unsigned dim)
{
  unsigned c = 0;
  for (unsigned i = 0; i < NTYPES; ++i)
    if (is_entity(Type(i)) && type_dims[i] == dim)
      c += count_of_type(objs, Type(i));
  return c;
}

void print_closure_dmg(FILE* f, Object* obj)
{
  auto closure = get_closure(obj, 0);
  fprintf(f, "%u %u %u %u\n",
      count_of_dim(closure, 3),
      count_of_dim(closure, 2),
      count_of_dim(closure, 1),
      count_of_dim(closure, 0));
  fprintf(f, "0 0 0\n0 0 0\n");
  for (auto it = closure.rbegin(); it != closure.rend(); ++it)
    print_object_dmg(f, *it);
}

void write_closure_to_dmg(Object* obj, char const* filename)
{
  FILE* f = fopen(filename, "w");
  print_closure_dmg(f, obj);
  fclose(f);
}

void add_use(Object* by, UseDir dir, Object* of)
{
  by->used.push_back(Use{dir, of});
  grab_object(of);
}

void add_helper(Object* to, Object* h)
{
  to->helpers.push_back(h);
  grab_object(h);
}

std::vector<Object*> get_closure(Object* obj, unsigned include_helpers)
{
  std::vector<Object*> queue;
  queue.reserve(nlive_objects);
  std::size_t first = 0;
  queue.push_back(obj);
  while (first != queue.size()) {
    Object* current = queue[first++];
    for (auto use : current->used) {
      auto child = use.obj;
      if (!child->visited) {
        child->visited = 1;
        queue.push_back(child);
      }
    }
    if (include_helpers)
      for (auto child : current->helpers) {
        if (!child->visited) {
          child->visited = 1;
          queue.push_back(child);
        }
      }
  }
  for (std::size_t i = 0; i < queue.size(); ++i)
    queue[i]->visited = 0;
  return queue;
}

Point* new_point(void)
{
  Point* p = new Point;
  init_object(&p->obj, POINT, free_object);
  return p;
}

double default_size = 0.1;

Point* new_point2(Vector v)
{
  Point* p = new_point();
  p->pos = v;
  p->size = default_size;
  return p;
}

Point* new_point3(Vector v, double size)
{
  Point* p = new_point();
  p->pos = v;
  p->size = size;
  return p;
}

std::vector<Point*> new_points(std::vector<Vector> vs)
{
  std::vector<Point*> out;
  for (auto vec : vs) out.push_back(new_point2(vec));
  return out;
}

void print_point(FILE* f, Point* p)
{
  fprintf(f, "Point(%u) = {%f,%f,%f,%f};\n",
      p->obj.id, p->pos.x, p->pos.y, p->pos.z, p->size);
}

Extruded extrude_point(Point* start, Vector v)
{
  Point* end = new_point3(add_vectors(start->pos, v), start->size);
  Object* middle = new_line2(start, end);
  return (Extruded){middle, &end->obj};
}

Point* edge_point(Object* edge, unsigned i)
{
  return (Point*) edge->used[i].obj;
}

Object* new_line(void)
{
  return new_object(LINE, free_object);
}

Object* new_line2(Point* start, Point* end)
{
  Object* l = new_line();
  add_use(l, FORWARD, &start->obj);
  add_use(l, FORWARD, &end->obj);
  return l;
}

Object* new_line3(Vector origin, Vector span)
{
  return extrude_point(new_point2(origin), span).middle;
}

Object* new_arc(void)
{
  return new_object(ARC, free_object);
}

Object* new_arc2(Point* start, Point* center,
    Point* end)
{
  Object* a = new_arc();
  add_use(a, FORWARD, &start->obj);
  add_helper(a, &center->obj);
  add_use(a, FORWARD, &end->obj);
  return a;
}

Point* arc_center(Object* arc)
{
  return (Point*) arc->helpers[0];
}

Vector arc_normal(Object* arc)
{
  return normalize_vector(
      cross_product(
        subtract_vectors(
          edge_point(arc, 0)->pos,
          arc_center(arc)->pos),
        subtract_vectors(
          edge_point(arc, 1)->pos,
          arc_center(arc)->pos)));
}

void print_arc(FILE* f, Object* arc)
{
  fprintf(f, "%s(%u) = {%u,%u,%u};\n", type_names[arc->type], arc->id,
      edge_point(arc, 0)->obj.id,
      arc_center(arc)->obj.id,
      edge_point(arc, 1)->obj.id);
}

Object* new_ellipse(void)
{
  return new_object(ELLIPSE, free_object);
}

Object* new_ellipse2(Point* start, Point* center,
    Point* major_pt, Point* end)
{
  Object* e = new_ellipse();
  add_use(e, FORWARD, &start->obj);
  add_helper(e, &center->obj);
  add_helper(e, &major_pt->obj);
  add_use(e, FORWARD, &end->obj);
  return e;
}

Point* ellipse_center(Object* e)
{
  return (Point*) e->helpers[0];
}

Point* ellipse_major_pt(Object* e)
{
  return (Point*) e->helpers[1];
}

void print_ellipse(FILE* f, Object* e)
{
  fprintf(f, "%s(%u) = {%u,%u,%u,%u};\n", type_names[e->type], e->id,
      edge_point(e, 0)->obj.id,
      ellipse_center(e)->obj.id,
      ellipse_major_pt(e)->obj.id,
      edge_point(e, 1)->obj.id);
}

Extruded extrude_edge(Object* start, Vector v)
{
  Extruded left = extrude_point(edge_point(start, 0), v);
  Extruded right = extrude_point(edge_point(start, 1), v);
  return extrude_edge2(start, v, left, right);
}

Extruded extrude_edge2(Object* start, Vector v,
    Extruded left, Extruded right)
{
  Object* loop = new_loop();
  add_use(loop, FORWARD, start);
  add_use(loop, FORWARD, right.middle);
  Object* end = 0;
  switch (start->type) {
    case LINE: {
      end = new_line2(
          (Point*) left.end,
          (Point*) right.end);
      break;
    }
    case ARC: {
      Point* start_center = arc_center(start);
      Point* end_center = new_point3(
          add_vectors(start_center->pos, v), start_center->size);
      end = new_arc2(
          (Point*) left.end,
          end_center,
          (Point*) right.end);
      break;
    }
    case ELLIPSE: {
      Point* start_center = ellipse_center(start);
      Point* end_center = new_point3(
          add_vectors(start_center->pos, v), start_center->size);
      Point* start_major_pt = ellipse_major_pt(start);
      Point* end_major_pt = new_point3(
          add_vectors(start_major_pt->pos, v), start_major_pt->size);
      end = new_ellipse2(
          (Point*) left.end,
          end_center,
          end_major_pt,
          (Point*) right.end);
      break;
    }
    default: end = 0; break;
  }
  add_use(loop, REVERSE, end);
  add_use(loop, REVERSE, left.middle);
  Object* middle;
  switch (start->type) {
    case LINE: middle = new_plane2(loop); break;
    case ARC:
    case ELLIPSE: middle = new_ruled2(loop); break;
    default: middle = 0; break;
  }
  return (Extruded){middle, end};
}

Object* new_loop(void)
{
  return new_object(LOOP, free_object);
}

std::vector<Point*> loop_points(Object* loop)
{
  std::vector<Point*> points;
  for (auto use : loop->used)
    points.push_back(edge_point(use.obj, use.dir));
  return points;
}

Extruded extrude_loop(Object* start, Vector v)
{
  Object* shell = new_shell();
  return extrude_loop2(start, v, shell, FORWARD);
}

Extruded extrude_loop2(Object* start, Vector v,
    Object* shell, UseDir shell_dir)
{
  Object* end = new_loop();
  std::vector<Point*> start_points = loop_points(start);
  auto n = start_points.size();
  std::vector<Extruded> point_extrusions;
  for (auto start_point : start_points)
    point_extrusions.push_back(extrude_point(start_point, v));
  std::vector<Extruded> edge_extrusions;
  for (std::size_t i = 0; i < n; ++i) {
    auto use = start->used[i];
    edge_extrusions.push_back(
        extrude_edge2(use.obj, v,
          point_extrusions[(i + (use.dir ^ 0)) % n],
          point_extrusions[(i + (use.dir ^ 1)) % n]));
  }
  for (std::size_t i = 0; i < n; ++i)
    add_use(end, start->used[i].dir, edge_extrusions[i].end);
  for (std::size_t i = 0; i < n; ++i)
    add_use(shell, start->used[i].dir ^ shell_dir, edge_extrusions[i].middle);
  return (Extruded){shell, end};
}

Object* new_circle(Vector center,
    Vector normal, Vector x)
{
  Matrix r = rotation_matrix(normal, PI / 2);
  Point* center_point = new_point2(center);
  Point* ring_points[4];
  for (unsigned i = 0; i < 4; ++i) {
    ring_points[i] = new_point2(add_vectors(center, x));
    x = matrix_vector_product(r, x);
  }
  Object* loop = new_loop();
  for (unsigned i = 0; i < 4; ++i) {
    Object* a = new_arc2(ring_points[i],
        center_point, ring_points[(i + 1) % 4]);
    add_use(loop, FORWARD, a);
  }
  return loop;
}

Object* new_polyline(Point** pts, unsigned npts)
{
  Object* loop = new_loop();
  for (unsigned i = 0; i < npts; ++i) {
    Object* line = new_line2(pts[i], pts[(i + 1) % npts]);
    add_use(loop, FORWARD, line);
  }
  return loop;
}

Object* new_polyline2(Vector* vs, unsigned npts)
{
  Point** pts = new_points(vs, npts);
  Object* out = new_polyline(pts, npts);
  free(pts);
  return out;
}

Object* new_plane(void)
{
  return new_object(PLANE, free_object);
}

Object* new_plane2(Object* loop)
{
  Object* p = new_plane();
  add_use(p, FORWARD, loop);
  return p;
}

Object* new_square(Vector origin,
    Vector x, Vector y)
{
  return extrude_edge(new_line3(origin, x), y).middle;
}

Object* new_disk(Vector center,
    Vector normal, Vector x)
{
  return new_plane2(new_circle(center, normal, x));
}

Object* new_polygon(Vector* vs, unsigned n)
{
  return new_plane2(new_polyline2(vs, n));
}

Object* new_ruled(void)
{
  return new_object(RULED, free_object);
}

Object* new_ruled2(Object* loop)
{
  Object* p = new_ruled();
  add_use(p, FORWARD, loop);
  return p;
}

void add_hole_to_face(Object* face, Object* loop)
{
  add_use(face, REVERSE, loop);
}

Extruded extrude_face(Object* face, Vector v)
{
  Object* end;
  switch (face->type) {
    case PLANE: end = new_plane(); break;
    case RULED: end = new_ruled(); break;
    default: end = 0; break;
  }
  Object* shell = new_shell();
  add_use(shell, REVERSE, face);
  add_use(shell, FORWARD, end);
  for (unsigned i = 0; i < face->nused; ++i) {
    Object* end_loop = extrude_loop2(face->used[i].obj, v,
        shell, face->used[i].dir).end;
    add_use(end, face->used[i].dir, end_loop);
  }
  Object* middle = new_volume2(shell);
  return (Extruded){middle, end};
}

Object* face_loop(Object* face)
{
  return face->used[0].obj;
}

Object* new_shell(void)
{
  return new_object(SHELL, free_object);
}

void make_hemisphere(Object* circle,
    Point* center, Object* shell,
    UseDir dir)
{
  assert(circle->nused == 4);
  Vector normal = arc_normal(circle->used[0].obj);
  if (dir == REVERSE)
    normal = scale_vector(-1, normal);
  Point** circle_points = loop_points(circle);
  double radius = vector_norm(
      subtract_vectors(circle_points[0]->pos, center->pos));
  Vector cap_pos = add_vectors(center->pos,
      scale_vector(radius, normal));
  Point* cap = new_point2(cap_pos);
  Object* inward[4];
  for (unsigned i = 0; i < 4; ++i)
    inward[i] = new_arc2(circle_points[i], center, cap);
  free(circle_points);
  for (unsigned i = 0; i < 4; ++i) {
    Object* loop = new_loop();
    add_use(loop, circle->used[i].dir ^ dir, circle->used[i].obj);
    add_use(loop, FORWARD ^ dir, inward[(i + 1) % 4]);
    add_use(loop, REVERSE ^ dir, inward[i]);
    add_use(shell, FORWARD, new_ruled2(loop));
  }
}

Object* new_sphere(Vector center,
    Vector normal, Vector x)
{
  Object* circle = new_circle(center, normal, x);
  Point* cpt = arc_center(circle->used[0].obj);
  Object* shell = new_shell();
  make_hemisphere(circle, cpt, shell, FORWARD);
  make_hemisphere(circle, cpt, shell, REVERSE);
  free_object(circle);
  return shell;
}

Object* new_volume(void)
{
  return new_object(VOLUME, free_object);
}

Object* new_volume2(Object* shell)
{
  Object* v = new_object(VOLUME, free_object);
  add_use(v, FORWARD, shell);
  return v;
}

Object* volume_shell(Object* v)
{
  return v->used[0].obj;
}

Object* new_cube(Vector origin,
    Vector x, Vector y, Vector z)
{
  return extrude_face(new_square(origin, x, y), z).middle;
}

Object* get_cube_face(Object* cube, enum cube_face which)
{
  return cube->used[0].obj->used[which].obj;
}

Object* new_ball(Vector center,
    Vector normal, Vector x)
{
  return new_volume2(new_sphere(center, normal, x));
}

void insert_into(Object* into, Object* o)
{
  add_use(into, REVERSE, o->used[0].obj);
}

Object* new_group(void)
{
  return new_object(GROUP, free_object);
}

void add_to_group(Object* group, Object* o)
{
  add_use(group, FORWARD, o);
}

void weld_volume_face_into(
    Object* big_volume,
    Object* small_volume,
    Object* big_volume_face,
    Object* small_volume_face)
{
  insert_into(big_volume_face, small_volume_face);
  add_use(volume_shell(big_volume),
      !(get_used_dir(volume_shell(small_volume), small_volume_face)),
      small_volume_face);
}

static int are_parallel(Vector a, Vector b)
{
  return 1e-6 > (1.0 - fabs(dot_product(normalize_vector(a),
                                        normalize_vector(b))));
}

static int are_perpendicular(Vector a, Vector b)
{
  return 1e-6 > (0.0 - fabs(dot_product(normalize_vector(a),
                                        normalize_vector(b))));
}

Vector eval(Object* o, double const* param)
{
  switch (o->type) {
    case POINT: {
      Point* p = (Point*) o;
      return p->pos;
    }
    case LINE: {
      double u = param[0];
      Point* a = edge_point(o, 0);
      Point* b = edge_point(o, 1);
      return add_vectors(scale_vector(1.0 - u, a->pos),
                         scale_vector(      u, b->pos));
    }
    case ARC: {
      double u = param[0];
      Point* a = edge_point(o, 0);
      Point* c = arc_center(o);
      Point* b = edge_point(o, 1);
      Vector n = arc_normal(o);
      Vector ca = subtract_vectors(a->pos, c->pos);
      Vector cb = subtract_vectors(b->pos, c->pos);
      double full_ang = acos(dot_product(ca, cb) /
          (vector_norm(ca) * vector_norm(cb)));
      double ang = full_ang * u;
      return rotate_vector(n, ang, ca);
    }
    case ELLIPSE: {
      double u = param[0];
      Point* a = edge_point(o, 0);
      Point* c = ellipse_center(o);
      Point* m = ellipse_major_pt(o);
      Point* b = edge_point(o, 1);
      Vector ca = subtract_vectors(a->pos, c->pos);
      Vector cb = subtract_vectors(b->pos, c->pos);
      Vector cm = subtract_vectors(m->pos, c->pos);
      if (!are_parallel(cb, cm)) {
        Point* tmp = a;
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
    default: return (Vector){-42,-42,-42};
  }
}

} // end namespace gmod
