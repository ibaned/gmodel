#ifndef GMODEL_HPP
#define GMODEL_HPP

#include <cmath>
#include <cstdio>
#include <vector>

namespace gmod {

constexpr double PI = 3.14159265359;

enum { NTYPES = 10 };

enum {
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

unsigned is_entity(int t);
unsigned is_boundary(int t);

enum {
  FORWARD = 0,
  REVERSE = 1,
};

struct Object;

struct Use {
  int dir;
  Object* obj;
};

struct Object {
  int type;
  unsigned id;
  unsigned ref_count;
  std::vector<Use> used;
  std::vector<Object*> helpers;
  void (*dtor)(Object* o);
  unsigned visited;
};

typedef void (*dtor_t)(Object* obj);

void init_object(Object* obj, int type, dtor_t dtor);
Object* new_object(int type, dtor_t dtor);
void free_object(Object* obj);
void grab_object(Object* obj);
void drop_object(Object* obj);

int get_used_dir(Object* user, Object* used);

void print_object(FILE* f, Object* obj);
void print_object_physical(FILE* f, Object* obj);
void print_closure(FILE* f, Object* obj);
void print_simple_object(FILE* f, Object* obj);

void write_closure_to_geo(Object* obj, char const* filename);

void print_object_dmg(FILE* f, Object* obj);
unsigned count_of_type(std::vector<Object*> const& objs, int type);
unsigned count_of_dim(std::vector<Object*> const& objs, unsigned dim);
void print_closure_dmg(FILE* f, Object* obj);

void write_closure_to_dmg(Object* obj, char const* filename);

void add_use(Object* by, int dir, Object* of);
void add_helper(Object* to, Object* h);
std::vector<Object*> get_closure(Object* obj, unsigned include_helpers);

struct Vector {double x, y, z;};
struct Matrix {Vector x, y, z;}; /* columns, not rows ! */

static inline Vector add_vectors(
    Vector a, Vector b)
{
  return (Vector){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vector subtract_vectors(
    Vector a, Vector b)
{
  return (Vector){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vector scale_vector(
    double a, Vector b)
{
  return (Vector){a * b.x, a * b.y, a * b.z};
}

static inline double dot_product(
    Vector a, Vector b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline double vector_norm(Vector a)
{
  return sqrt(dot_product(a, a));
}

static inline Vector normalize_vector(Vector v)
{
  return scale_vector(1.0 / vector_norm(v), v);
}

static inline Vector matrix_vector_product(
    Matrix a, Vector b)
{
  return add_vectors(scale_vector(b.x, a.x),
         add_vectors(scale_vector(b.y, a.y),
                     scale_vector(b.z, a.z)));
}

static inline Matrix cross_product_matrix(
    Vector a)
{
  return (Matrix){
    (Vector){    0,  a.z, -a.y},
    (Vector){ -a.z,    0,  a.x},
    (Vector){  a.y, -a.x,    0}};
}

static inline Vector cross_product(
    Vector a, Vector b)
{
  return matrix_vector_product(
      cross_product_matrix(a), b);
}

static inline Matrix tensor_product_matrix(
    Vector a, Vector b)
{
  return (Matrix){
    scale_vector(b.x, a),
    scale_vector(b.y, a),
    scale_vector(b.z, a)};
}

static inline Matrix identity_matrix(void)
{
  return (Matrix){
    (Vector){1,0,0},
    (Vector){0,1,0},
    (Vector){0,0,1}};
}

static inline Matrix scale_matrix(double a,
    Matrix b)
{
  return (Matrix){
    scale_vector(a, b.x),
    scale_vector(a, b.y),
    scale_vector(a, b.z)};
}

static inline Matrix add_matrices(
    Matrix a, Matrix b)
{
  return (Matrix){
    add_vectors(a.x, b.x),
    add_vectors(a.y, b.y),
    add_vectors(a.z, b.z)};
}

static inline Matrix rotation_matrix(Vector axis, double angle)
{
  return add_matrices(scale_matrix(cos(angle), identity_matrix()),
         add_matrices(scale_matrix(sin(angle), cross_product_matrix(axis)),
                      scale_matrix(1 - cos(angle),
                                   tensor_product_matrix(axis, axis))));
}

static inline Vector rotate_vector(Vector axis, double angle,
    Vector v)
{
  return matrix_vector_product(rotation_matrix(axis, angle), v);
}

struct Point {
  Object obj;
  Vector pos;
  double size;
};

extern double default_size;

Point* new_point(void);
Point* new_point2(Vector v);
Point* new_point3(Vector v, double size);
std::vector<Point*> new_points(std::vector<Vector> vs);

void print_point(FILE* f, Point* p);

struct Extruded {
  Object* middle;
  Object* end;
};

Extruded extrude_point(Point* start, Vector v);
Point* edge_point(Object* edge, unsigned i);

Object* new_line(void);
Object* new_line2(Point* start, Point* end);
Object* new_line3(Vector origin, Vector span);

Object* new_arc(void);
Object* new_arc2(Point* start, Point* center,
    Point* end);
Point* arc_center(Object* arc);
Vector arc_normal(Object* arc);
void print_arc(FILE* f, Object* arc);

Object* new_ellipse(void);
Object* new_ellipse2(Point* start, Point* center,
    Point* major_pt, Point* end);
Point* ellipse_center(Object* e);
Point* ellipse_major_pt(Object* e);
void print_ellipse(FILE* f, Object* e);

Extruded extrude_edge(Object* start, Vector v);
Extruded extrude_edge2(Object* start, Vector v,
    Extruded left, Extruded right);

Object* new_loop(void);
std::vector<Point*> loop_points(Object* loop);
Extruded extrude_loop(Object* start, Vector v);
Extruded extrude_loop2(Object* start, Vector v,
    Object* shell, int shell_dir);

Object* new_circle(Vector center,
    Vector normal, Vector x);
Object* new_polyline(Point** pts, unsigned npts);
Object* new_polyline2(Vector* vs, unsigned npts);

Object* new_plane(void);
Object* new_plane2(Object* loop);

Object* new_square(Vector origin,
    Vector x, Vector y);
Object* new_disk(Vector center,
    Vector normal, Vector x);
Object* new_polygon(Vector* vs, unsigned n);

Object* new_ruled(void);
Object* new_ruled2(Object* loop);

void add_hole_to_face(Object* face, Object* loop);
Extruded extrude_face(Object* face, Vector v);
Object* face_loop(Object* face);

Object* new_shell(void);

void make_hemisphere(Object* circle,
    Point* center, Object* shell,
    int dir);
Object* new_sphere(Vector center,
    Vector normal, Vector x);

Object* new_volume(void);
Object* new_volume2(Object* shell);
Object* volume_shell(Object* v);

Object* new_cube(Vector origin,
    Vector x, Vector y, Vector z);

enum cube_face {
  BOTTOM,
  TOP,
  FRONT,
  RIGHT,
  BACK,
  LEFT
};

Object* get_cube_face(Object* cube, enum cube_face which);

Object* new_ball(Vector center,
    Vector normal, Vector x);

void insert_into(Object* into, Object* f);

Object* new_group(void);
void add_to_group(Object* group, Object* o);

void weld_volume_face_into(
    Object* big_volume,
    Object* small_volume,
    Object* big_volume_face,
    Object* small_volume_face);

Vector eval(Object* o, double const* param);

} // end namespace gmod

#endif
