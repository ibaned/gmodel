#include "gmodel.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

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

unsigned is_entity(enum type t)
{
  return t <= VOLUME;
}

unsigned is_boundary(enum type t)
{
  return t == LOOP || t == SHELL;
}

static unsigned next_id = 0;
static unsigned nlive_objects = 0;

void init_object(struct object* obj, enum type type, dtor_t dtor)
{
  obj->type = type;
  obj->id = next_id++;
  obj->ref_count = 0;
  obj->nused = 0;
  obj->used = 0;
  obj->nhelpers = 0;
  obj->helpers = 0;
  obj->dtor = dtor;
  obj->visited = 0;
  ++nlive_objects;
}

struct object* new_object(enum type type, dtor_t dtor)
{
  struct object* o = malloc(sizeof(*o));
  init_object(o, type, dtor);
  return o;
}

void free_object(struct object* obj)
{
  for (unsigned i = 0; i < obj->nused; ++i)
    drop_object(obj->used[i].obj);
  free(obj->used);
  for (unsigned i = 0; i < obj->nhelpers; ++i)
    drop_object(obj->helpers[i]);
  free(obj->helpers);
  free(obj);
  --nlive_objects;
}

void grab_object(struct object* obj)
{
  ++obj->ref_count;
}

void drop_object(struct object* obj)
{
  assert(obj->ref_count);
  --obj->ref_count;
  if (!obj->ref_count)
    obj->dtor(obj);
}

enum use_dir get_used_dir(struct object* user, struct object* used)
{
  for (unsigned i = 0; i < user->nused; ++i)
    if (user->used[i].obj == used)
      return user->used[i].dir;
  assert(0);
}

void print_object(FILE* f, struct object* obj)
{
  switch (obj->type) {
    case POINT: print_point(f, (struct point*)obj); break;
    case ARC: print_arc(f, obj); break;
    case ELLIPSE: print_ellipse(f, obj); break;
    case GROUP: break;
    default: print_simple_object(f, obj); break;
  }
}

void print_object_physical(FILE* f, struct object* obj)
{
  if (!is_entity(obj->type))
    return;
  fprintf(f, "%s(%u) = {%u};\n", physical_type_names[obj->type],
      obj->id, obj->id);
}

void print_closure(FILE* f, struct object* obj)
{
  struct object** closure;
  unsigned closure_size;
  get_closure(obj, 1, &closure, &closure_size);
  for (unsigned i = 0; i < closure_size; ++i)
    print_object(f, closure[closure_size - i - 1]);
  free(closure);
  get_closure(obj, 0, &closure, &closure_size);
  for (unsigned i = 0; i < closure_size; ++i)
    print_object_physical(f, closure[closure_size - i - 1]);
  free(closure);
}

void write_closure_to_geo(struct object* obj, char const* filename)
{
  FILE* f = fopen(filename, "w");
  print_closure(f, obj);
  fclose(f);
}

void print_simple_object(FILE* f, struct object* obj)
{
  fprintf(f, "%s(%u) = {", type_names[obj->type], obj->id);
  for (unsigned i = 0; i < obj->nused; ++i) {
    if (i)
      fprintf(f, ",");
    if (is_boundary(obj->type) && obj->used[i].dir == REVERSE)
      fprintf(f, "%d", -((int)(obj->used[i].obj->id)));
    else
      fprintf(f, "%u", obj->used[i].obj->id);
  }
  fprintf(f, "};\n");
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#endif

void print_object_dmg(FILE* f, struct object* obj)
{
  switch (obj->type) {
    case POINT: {
      struct point* p = (struct point*) obj;
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
      fprintf(f, "%u %u\n", obj->id, obj->nused);
      for (unsigned i = 0; i < obj->nused; ++i) {
        struct object* loop = obj->used[i].obj;
        fprintf(f, " %u\n", loop->nused);
        for (unsigned j = 0; j < loop->nused; ++j) {
          struct use u = loop->used[j];
          fprintf(f, "  %u %u\n", u.obj->id, !u.dir);
        }
      }
    } break;
  }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

unsigned count_of_type(struct object** objs, unsigned n, enum type type)
{
  unsigned c = 0;
  for (unsigned i = 0; i < n; ++i)
    if (objs[i]->type == type)
      ++c;
  return c;
}

unsigned count_of_dim(struct object** objs, unsigned n, unsigned dim)
{
  unsigned c = 0;
  for (unsigned i = 0; i < NTYPES; ++i)
    if (is_entity(i) && type_dims[i] == dim)
      c += count_of_type(objs, n, i);
  return c;
}

void print_closure_dmg(FILE* f, struct object* obj)
{
  struct object** closure;
  unsigned closure_size;
  get_closure(obj, 0, &closure, &closure_size);
  fprintf(f, "%u %u %u %u\n",
      count_of_dim(closure, closure_size, 3),
      count_of_dim(closure, closure_size, 2),
      count_of_dim(closure, closure_size, 1),
      count_of_dim(closure, closure_size, 0));
  fprintf(f, "0 0 0\n0 0 0\n");
  for (unsigned i = 0; i < closure_size; ++i)
    print_object_dmg(f, closure[closure_size - i - 1]);
  free(closure);
}

void write_closure_to_dmg(struct object* obj, char const* filename)
{
  FILE* f = fopen(filename, "w");
  print_closure_dmg(f, obj);
  fclose(f);
}

void add_use(struct object* by, enum use_dir dir, struct object* of)
{
  ++by->nused;
  by->used = realloc(by->used, by->nused * sizeof(struct use));
  by->used[by->nused - 1] = (struct use){dir, of};
  grab_object(of);
}

void add_helper(struct object* to, struct object* h)
{
  ++to->nhelpers;
  to->helpers = realloc(to->helpers, to->nhelpers * sizeof(struct object*));
  to->helpers[to->nhelpers - 1] = h;
  grab_object(h);
}

void get_closure(struct object* obj, unsigned include_helpers,
    struct object*** p_objs, unsigned* p_count)
{
  struct object** queue = malloc(nlive_objects * sizeof(struct object*));
  unsigned first = 0;
  unsigned end = 0;
  queue[end++] = obj;
  while (first != end) {
    struct object* current = queue[first++];
    for (unsigned i = 0; i < current->nused; ++i) {
      struct object* child = current->used[i].obj;
      if (!child->visited) {
        child->visited = 1;
        queue[end++] = child;
      }
    }
    if (include_helpers)
      for (unsigned i = 0; i < current->nhelpers; ++i) {
        struct object* child = current->helpers[i];
        if (!child->visited) {
          child->visited = 1;
          queue[end++] = child;
        }
      }
  }
  for (unsigned i = 0; i < end; ++i)
    queue[i]->visited = 0;
  *p_objs = realloc(queue, end * sizeof(struct object*));
  *p_count = end;
}

struct point* new_point(void)
{
  struct point* p = malloc(sizeof(struct point));
  init_object(&p->obj, POINT, free_object);
  return p;
}

double default_size = 0.1;

struct point* new_point2(struct vector v)
{
  struct point* p = new_point();
  p->pos = v;
  p->size = default_size;
  return p;
}

struct point* new_point3(struct vector v, double size)
{
  struct point* p = new_point();
  p->pos = v;
  p->size = size;
  return p;
}

struct point** new_points(struct vector* vs, unsigned n)
{
  struct point** out = malloc(n * sizeof(struct point*));
  for (unsigned i = 0; i < n; ++i)
    out[i] = new_point2(vs[i]);
  return out;
}

void print_point(FILE* f, struct point* p)
{
  fprintf(f, "Point(%u) = {%f,%f,%f,%f};\n",
      p->obj.id, p->pos.x, p->pos.y, p->pos.z, p->size);
}

struct extruded extrude_point(struct point* start, struct vector v)
{
  struct point* end = new_point3(add_vectors(start->pos, v), start->size);
  struct object* middle = new_line2(start, end);
  return (struct extruded){middle, &end->obj};
}

struct point* edge_point(struct object* edge, unsigned i)
{
  return (struct point*) edge->used[i].obj;
}

struct object* new_line(void)
{
  return new_object(LINE, free_object);
}

struct object* new_line2(struct point* start, struct point* end)
{
  struct object* l = new_line();
  add_use(l, FORWARD, &start->obj);
  add_use(l, FORWARD, &end->obj);
  return l;
}

struct object* new_line3(struct vector origin, struct vector span)
{
  return extrude_point(new_point2(origin), span).middle;
}

struct object* new_arc(void)
{
  return new_object(ARC, free_object);
}

struct object* new_arc2(struct point* start, struct point* center,
    struct point* end)
{
  struct object* a = new_arc();
  add_use(a, FORWARD, &start->obj);
  add_helper(a, &center->obj);
  add_use(a, FORWARD, &end->obj);
  return a;
}

struct point* arc_center(struct object* arc)
{
  return (struct point*) arc->helpers[0];
}

struct vector arc_normal(struct object* arc)
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

void print_arc(FILE* f, struct object* arc)
{
  fprintf(f, "%s(%u) = {%u,%u,%u};\n", type_names[arc->type], arc->id,
      edge_point(arc, 0)->obj.id,
      arc_center(arc)->obj.id,
      edge_point(arc, 1)->obj.id);
}

struct object* new_ellipse(void)
{
  return new_object(ELLIPSE, free_object);
}

struct object* new_ellipse2(struct point* start, struct point* center,
    struct point* major_pt, struct point* end)
{
  struct object* e = new_ellipse();
  add_use(e, FORWARD, &start->obj);
  add_helper(e, &center->obj);
  add_helper(e, &major_pt->obj);
  add_use(e, FORWARD, &end->obj);
  return e;
}

struct point* ellipse_center(struct object* e)
{
  return (struct point*) e->helpers[0];
}

struct point* ellipse_major_pt(struct object* e)
{
  return (struct point*) e->helpers[1];
}

void print_ellipse(FILE* f, struct object* e)
{
  fprintf(f, "%s(%u) = {%u,%u,%u,%u};\n", type_names[e->type], e->id,
      edge_point(e, 0)->obj.id,
      ellipse_center(e)->obj.id,
      ellipse_major_pt(e)->obj.id,
      edge_point(e, 1)->obj.id);
}

struct extruded extrude_edge(struct object* start, struct vector v)
{
  struct extruded left = extrude_point(edge_point(start, 0), v);
  struct extruded right = extrude_point(edge_point(start, 1), v);
  return extrude_edge2(start, v, left, right);
}

struct extruded extrude_edge2(struct object* start, struct vector v,
    struct extruded left, struct extruded right)
{
  struct object* loop = new_loop();
  add_use(loop, FORWARD, start);
  add_use(loop, FORWARD, right.middle);
  struct object* end = 0;
  switch (start->type) {
    case LINE: {
      end = new_line2(
          (struct point*) left.end,
          (struct point*) right.end);
      break;
    }
    case ARC: {
      struct point* start_center = arc_center(start);
      struct point* end_center = new_point3(
          add_vectors(start_center->pos, v), start_center->size);
      end = new_arc2(
          (struct point*) left.end,
          end_center,
          (struct point*) right.end);
      break;
    }
    case ELLIPSE: {
      struct point* start_center = ellipse_center(start);
      struct point* end_center = new_point3(
          add_vectors(start_center->pos, v), start_center->size);
      struct point* start_major_pt = ellipse_major_pt(start);
      struct point* end_major_pt = new_point3(
          add_vectors(start_major_pt->pos, v), start_major_pt->size);
      end = new_ellipse2(
          (struct point*) left.end,
          end_center,
          end_major_pt,
          (struct point*) right.end);
      break;
    }
    default: end = 0; break;
  }
  add_use(loop, REVERSE, end);
  add_use(loop, REVERSE, left.middle);
  struct object* middle;
  switch (start->type) {
    case LINE: middle = new_plane2(loop); break;
    case ARC:
    case ELLIPSE: middle = new_ruled2(loop); break;
    default: middle = 0; break;
  }
  return (struct extruded){middle, end};
}

struct object* new_loop(void)
{
  return new_object(LOOP, free_object);
}

struct point** loop_points(struct object* loop)
{
  struct point** points = malloc(sizeof(struct point*) * loop->nused);
  for (unsigned i = 0; i < loop->nused; ++i)
    points[i] = edge_point(loop->used[i].obj, loop->used[i].dir);
  return points;
}

struct extruded extrude_loop(struct object* start, struct vector v)
{
  struct object* shell = new_shell();
  return extrude_loop2(start, v, shell, FORWARD);
}

struct extruded extrude_loop2(struct object* start, struct vector v,
    struct object* shell, enum use_dir shell_dir)
{
  struct object* end = new_loop();
  struct point** start_points = loop_points(start);
  struct extruded* point_extrusions = malloc(
      sizeof(struct extruded) * start->nused);
  for (unsigned i = 0; i < start->nused; ++i)
    point_extrusions[i] = extrude_point(start_points[i], v);
  free(start_points);
  struct extruded* edge_extrusions = malloc(
      sizeof(struct extruded) * start->nused);
  for (unsigned i = 0; i < start->nused; ++i)
    edge_extrusions[i] = extrude_edge2(start->used[i].obj, v,
        point_extrusions[(i + (start->used[i].dir ^ 0)) % start->nused],
        point_extrusions[(i + (start->used[i].dir ^ 1)) % start->nused]);
  free(point_extrusions);
  for (unsigned i = 0; i < start->nused; ++i)
    add_use(end, start->used[i].dir, edge_extrusions[i].end);
  for (unsigned i = 0; i < start->nused; ++i)
    add_use(shell, start->used[i].dir ^ shell_dir, edge_extrusions[i].middle);
  free(edge_extrusions);
  return (struct extruded){shell, end};
}

struct object* new_circle(struct vector center,
    struct vector normal, struct vector x)
{
  struct matrix r = rotation_matrix(normal, PI / 2);
  struct point* center_point = new_point2(center);
  struct point* ring_points[4];
  for (unsigned i = 0; i < 4; ++i) {
    ring_points[i] = new_point2(add_vectors(center, x));
    x = matrix_vector_product(r, x);
  }
  struct object* loop = new_loop();
  for (unsigned i = 0; i < 4; ++i) {
    struct object* a = new_arc2(ring_points[i],
        center_point, ring_points[(i + 1) % 4]);
    add_use(loop, FORWARD, a);
  }
  return loop;
}

struct object* new_polyline(struct point** pts, unsigned npts)
{
  struct object* loop = new_loop();
  for (unsigned i = 0; i < npts; ++i) {
    struct object* line = new_line2(pts[i], pts[(i + 1) % npts]);
    add_use(loop, FORWARD, line);
  }
  return loop;
}

struct object* new_polyline2(struct vector* vs, unsigned npts)
{
  struct point** pts = new_points(vs, npts);
  struct object* out = new_polyline(pts, npts);
  free(pts);
  return out;
}

struct object* new_plane(void)
{
  return new_object(PLANE, free_object);
}

struct object* new_plane2(struct object* loop)
{
  struct object* p = new_plane();
  add_use(p, FORWARD, loop);
  return p;
}

struct object* new_square(struct vector origin,
    struct vector x, struct vector y)
{
  return extrude_edge(new_line3(origin, x), y).middle;
}

struct object* new_disk(struct vector center,
    struct vector normal, struct vector x)
{
  return new_plane2(new_circle(center, normal, x));
}

struct object* new_polygon(struct vector* vs, unsigned n)
{
  return new_plane2(new_polyline2(vs, n));
}

struct object* new_ruled(void)
{
  return new_object(RULED, free_object);
}

struct object* new_ruled2(struct object* loop)
{
  struct object* p = new_ruled();
  add_use(p, FORWARD, loop);
  return p;
}

void add_hole_to_face(struct object* face, struct object* loop)
{
  add_use(face, REVERSE, loop);
}

struct extruded extrude_face(struct object* face, struct vector v)
{
  struct object* end;
  switch (face->type) {
    case PLANE: end = new_plane(); break;
    case RULED: end = new_ruled(); break;
    default: end = 0; break;
  }
  struct object* shell = new_shell();
  add_use(shell, REVERSE, face);
  add_use(shell, FORWARD, end);
  for (unsigned i = 0; i < face->nused; ++i) {
    struct object* end_loop = extrude_loop2(face->used[i].obj, v,
        shell, face->used[i].dir).end;
    add_use(end, face->used[i].dir, end_loop);
  }
  struct object* middle = new_volume2(shell);
  return (struct extruded){middle, end};
}

struct object* face_loop(struct object* face)
{
  return face->used[0].obj;
}

struct object* new_shell(void)
{
  return new_object(SHELL, free_object);
}

void make_hemisphere(struct object* circle,
    struct point* center, struct object* shell,
    enum use_dir dir)
{
  assert(circle->nused == 4);
  struct vector normal = arc_normal(circle->used[0].obj);
  if (dir == REVERSE)
    normal = scale_vector(-1, normal);
  struct point** circle_points = loop_points(circle);
  double radius = vector_norm(
      subtract_vectors(circle_points[0]->pos, center->pos));
  struct vector cap_pos = add_vectors(center->pos,
      scale_vector(radius, normal));
  struct point* cap = new_point2(cap_pos);
  struct object* inward[4];
  for (unsigned i = 0; i < 4; ++i)
    inward[i] = new_arc2(circle_points[i], center, cap);
  free(circle_points);
  for (unsigned i = 0; i < 4; ++i) {
    struct object* loop = new_loop();
    add_use(loop, circle->used[i].dir ^ dir, circle->used[i].obj);
    add_use(loop, FORWARD ^ dir, inward[(i + 1) % 4]);
    add_use(loop, REVERSE ^ dir, inward[i]);
    add_use(shell, FORWARD, new_ruled2(loop));
  }
}

struct object* new_sphere(struct vector center,
    struct vector normal, struct vector x)
{
  struct object* circle = new_circle(center, normal, x);
  struct point* cpt = arc_center(circle->used[0].obj);
  struct object* shell = new_shell();
  make_hemisphere(circle, cpt, shell, FORWARD);
  make_hemisphere(circle, cpt, shell, REVERSE);
  free_object(circle);
  return shell;
}

struct object* new_volume(void)
{
  return new_object(VOLUME, free_object);
}

struct object* new_volume2(struct object* shell)
{
  struct object* v = new_object(VOLUME, free_object);
  add_use(v, FORWARD, shell);
  return v;
}

struct object* volume_shell(struct object* v)
{
  return v->used[0].obj;
}

struct object* new_cube(struct vector origin,
    struct vector x, struct vector y, struct vector z)
{
  return extrude_face(new_square(origin, x, y), z).middle;
}

struct object* get_cube_face(struct object* cube, enum cube_face which)
{
  return cube->used[0].obj->used[which].obj;
}

struct object* new_ball(struct vector center,
    struct vector normal, struct vector x)
{
  return new_volume2(new_sphere(center, normal, x));
}

void insert_into(struct object* into, struct object* o)
{
  add_use(into, REVERSE, o->used[0].obj);
}

struct object* new_group(void)
{
  return new_object(GROUP, free_object);
}

void add_to_group(struct object* group, struct object* o)
{
  add_use(group, FORWARD, o);
}

void weld_volume_face_into(
    struct object* big_volume,
    struct object* small_volume,
    struct object* big_volume_face,
    struct object* small_volume_face)
{
  insert_into(big_volume_face, small_volume_face);
  add_use(volume_shell(big_volume),
      !(get_used_dir(volume_shell(small_volume), small_volume_face)),
      small_volume_face);
}

static int are_parallel(struct vector a, struct vector b)
{
  return 1e-6 > (1.0 - fabs(dot_product(normalize_vector(a),
                                        normalize_vector(b))));
}

static int are_perpendicular(struct vector a, struct vector b)
{
  return 1e-6 > (0.0 - fabs(dot_product(normalize_vector(a),
                                        normalize_vector(b))));
}

struct vector eval(struct object* o, double const* param)
{
  switch (o->type) {
    case POINT: {
      struct point* p = (struct point*) o;
      return p->pos;
    }
    case LINE: {
      double u = param[0];
      struct point* a = edge_point(o, 0);
      struct point* b = edge_point(o, 1);
      return add_vectors(scale_vector(1.0 - u, a->pos),
                         scale_vector(      u, b->pos));
    }
    case ARC: {
      double u = param[0];
      struct point* a = edge_point(o, 0);
      struct point* c = arc_center(o);
      struct point* b = edge_point(o, 1);
      struct vector n = arc_normal(o);
      struct vector ca = subtract_vectors(a->pos, c->pos);
      struct vector cb = subtract_vectors(b->pos, c->pos);
      double full_ang = acos(dot_product(ca, cb) /
          (vector_norm(ca) * vector_norm(cb)));
      double ang = full_ang * u;
      return rotate_vector(n, ang, ca);
    }
    case ELLIPSE: {
      double u = param[0];
      struct point* a = edge_point(o, 0);
      struct point* c = ellipse_center(o);
      struct point* m = ellipse_major_pt(o);
      struct point* b = edge_point(o, 1);
      struct vector ca = subtract_vectors(a->pos, c->pos);
      struct vector cb = subtract_vectors(b->pos, c->pos);
      struct vector cm = subtract_vectors(m->pos, c->pos);
      if (!are_parallel(cb, cm)) {
        struct point* tmp = a;
        a = b;
        b = tmp;
        u = 1.0 - u;
        if (!are_parallel(cb, cm)) {
          fprintf(stderr, "scimod only understands quarter ellipses,\n");
          fprintf(stderr, "and this one has no endpoint on the major axis\n");
          abort();
        }
        if (!are_perpendicular(ca, cm)) {
          fprintf(stderr, "scimod only understands quarter ellipses,\n");
          fprintf(stderr, "and this one has no endpoint on the minor axis\n");
          abort();
        }
      }
      double full_ang = PI / 2.0;
      double ang = full_ang * u;
      return add_vectors(c->pos, add_vectors(scale_vector(cos(ang), ca),
                                             scale_vector(sin(ang), cb)));
    }
    default: return (struct vector){-42,-42,-42};
  }
}
