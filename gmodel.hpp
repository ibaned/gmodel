#ifndef GMODEL_HPP
#define GMODEL_HPP

#include <math.h>
#include <stdio.h>

namespace gmod {

constexpr double PI = 3.14159265359;

enum { NTYPES = 10 };

enum type {
  POINT    = 0,
  LINE     = 1,
  ARC      = 2,
  ELLIPSE  = 3,
  PLANE    = 4,
  RULED    = 5,
  VOLUME   = 6,
  LOOP     = 7,
  SHELL    = 8,
  GROUP    = 9
};

extern char const* const type_names[NTYPES];
extern char const* const physical_type_names[NTYPES];
extern unsigned const type_dims[NTYPES];

unsigned is_entity(enum type t);
unsigned is_boundary(enum type t);

enum use_dir {
  FORWARD = 0,
  REVERSE = 1,
};

struct use {
  enum use_dir dir;
  struct object* obj;
};

struct object {
  enum type type;
  unsigned id;
  unsigned ref_count;
  unsigned nused;
  struct use* used;
  unsigned nhelpers;
  struct object** helpers;
  void (*dtor)(struct object* o);
  unsigned visited;
};

typedef void (*dtor_t)(struct object* obj);

void init_object(struct object* obj, enum type type, dtor_t dtor);
struct object* new_object(enum type type, dtor_t dtor);
void free_object(struct object* obj);
void grab_object(struct object* obj);
void drop_object(struct object* obj);

enum use_dir get_used_dir(struct object* user, struct object* used);

void print_object(FILE* f, struct object* obj);
void print_object_physical(FILE* f, struct object* obj);
void print_closure(FILE* f, struct object* obj);
void print_simple_object(FILE* f, struct object* obj);

void write_closure_to_geo(struct object* obj, char const* filename);

void print_object_dmg(FILE* f, struct object* obj);
unsigned count_of_type(struct object** objs, unsigned n, enum type type);
unsigned count_of_dim(struct object** objs, unsigned n, unsigned dim);
void print_closure_dmg(FILE* f, struct object* obj);

void write_closure_to_dmg(struct object* obj, char const* filename);

void add_use(struct object* by, enum use_dir dir, struct object* of);
void add_helper(struct object* to, struct object* h);
void get_closure(struct object* obj, unsigned include_helpers,
    struct object*** p_objs, unsigned* p_count);

struct vector {double x, y, z;};
struct matrix {struct vector x, y, z;}; /* columns, not rows ! */

static inline struct vector add_vectors(
    struct vector a, struct vector b)
{
  return (struct vector){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline struct vector subtract_vectors(
    struct vector a, struct vector b)
{
  return (struct vector){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline struct vector scale_vector(
    double a, struct vector b)
{
  return (struct vector){a * b.x, a * b.y, a * b.z};
}

static inline double dot_product(
    struct vector a, struct vector b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline double vector_norm(struct vector a)
{
  return sqrt(dot_product(a, a));
}

static inline struct vector normalize_vector(struct vector v)
{
  return scale_vector(1.0 / vector_norm(v), v);
}

static inline struct vector matrix_vector_product(
    struct matrix a, struct vector b)
{
  return add_vectors(scale_vector(b.x, a.x),
         add_vectors(scale_vector(b.y, a.y),
                     scale_vector(b.z, a.z)));
}

static inline struct matrix cross_product_matrix(
    struct vector a)
{
  return (struct matrix){
    (struct vector){    0,  a.z, -a.y},
    (struct vector){ -a.z,    0,  a.x},
    (struct vector){  a.y, -a.x,    0}};
}

static inline struct vector cross_product(
    struct vector a, struct vector b)
{
  return matrix_vector_product(
      cross_product_matrix(a), b);
}

static inline struct matrix tensor_product_matrix(
    struct vector a, struct vector b)
{
  return (struct matrix){
    scale_vector(b.x, a),
    scale_vector(b.y, a),
    scale_vector(b.z, a)};
}

static inline struct matrix identity_matrix(void)
{
  return (struct matrix){
    (struct vector){1,0,0},
    (struct vector){0,1,0},
    (struct vector){0,0,1}};
}

static inline struct matrix scale_matrix(double a,
    struct matrix b)
{
  return (struct matrix){
    scale_vector(a, b.x),
    scale_vector(a, b.y),
    scale_vector(a, b.z)};
}

static inline struct matrix add_matrices(
    struct matrix a, struct matrix b)
{
  return (struct matrix){
    add_vectors(a.x, b.x),
    add_vectors(a.y, b.y),
    add_vectors(a.z, b.z)};
}

static inline struct matrix rotation_matrix(struct vector axis, double angle)
{
  return add_matrices(scale_matrix(cos(angle), identity_matrix()),
         add_matrices(scale_matrix(sin(angle), cross_product_matrix(axis)),
                      scale_matrix(1 - cos(angle),
                                   tensor_product_matrix(axis, axis))));
}

static inline struct vector rotate_vector(struct vector axis, double angle,
    struct vector v)
{
  return matrix_vector_product(rotation_matrix(axis, angle), v);
}

struct point {
  struct object obj;
  struct vector pos;
  double size;
};

extern double default_size;

struct point* new_point(void);
struct point* new_point2(struct vector v);
struct point* new_point3(struct vector v, double size);
struct point** new_points(struct vector* vs, unsigned n);

void print_point(FILE* f, struct point* p);

struct extruded {
  struct object* middle;
  struct object* end;
};

struct extruded extrude_point(struct point* start, struct vector v);
struct point* edge_point(struct object* edge, unsigned i);

struct object* new_line(void);
struct object* new_line2(struct point* start, struct point* end);
struct object* new_line3(struct vector origin, struct vector span);

struct object* new_arc(void);
struct object* new_arc2(struct point* start, struct point* center,
    struct point* end);
struct point* arc_center(struct object* arc);
struct vector arc_normal(struct object* arc);
void print_arc(FILE* f, struct object* arc);

struct object* new_ellipse(void);
struct object* new_ellipse2(struct point* start, struct point* center,
    struct point* major_pt, struct point* end);
struct point* ellipse_center(struct object* e);
struct point* ellipse_major_pt(struct object* e);
void print_ellipse(FILE* f, struct object* e);

struct extruded extrude_edge(struct object* start, struct vector v);
struct extruded extrude_edge2(struct object* start, struct vector v,
    struct extruded left, struct extruded right);

struct object* new_loop(void);
struct point** loop_points(struct object* loop);
struct extruded extrude_loop(struct object* start, struct vector v);
struct extruded extrude_loop2(struct object* start, struct vector v,
    struct object* shell, enum use_dir shell_dir);

struct object* new_circle(struct vector center,
    struct vector normal, struct vector x);
struct object* new_polyline(struct point** pts, unsigned npts);
struct object* new_polyline2(struct vector* vs, unsigned npts);

struct object* new_plane(void);
struct object* new_plane2(struct object* loop);

struct object* new_square(struct vector origin,
    struct vector x, struct vector y);
struct object* new_disk(struct vector center,
    struct vector normal, struct vector x);
struct object* new_polygon(struct vector* vs, unsigned n);

struct object* new_ruled(void);
struct object* new_ruled2(struct object* loop);

void add_hole_to_face(struct object* face, struct object* loop);
struct extruded extrude_face(struct object* face, struct vector v);
struct object* face_loop(struct object* face);

struct object* new_shell(void);

void make_hemisphere(struct object* circle,
    struct point* center, struct object* shell,
    enum use_dir dir);
struct object* new_sphere(struct vector center,
    struct vector normal, struct vector x);

struct object* new_volume(void);
struct object* new_volume2(struct object* shell);
struct object* volume_shell(struct object* v);

struct object* new_cube(struct vector origin,
    struct vector x, struct vector y, struct vector z);

enum cube_face {
  BOTTOM,
  TOP,
  FRONT,
  RIGHT,
  BACK,
  LEFT
};

struct object* get_cube_face(struct object* cube, enum cube_face which);

struct object* new_ball(struct vector center,
    struct vector normal, struct vector x);

void insert_into(struct object* into, struct object* f);

struct object* new_group(void);
void add_to_group(struct object* group, struct object* o);

void weld_volume_face_into(
    struct object* big_volume,
    struct object* small_volume,
    struct object* big_volume_face,
    struct object* small_volume_face);

struct vector eval(struct object* o, double const* param);

} // end namespace gmod

#endif
