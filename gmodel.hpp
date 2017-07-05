#ifndef GMODEL_HPP
#define GMODEL_HPP

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
#include <functional>

namespace gmod {

constexpr double PI = 3.14159265359;

enum { NTYPES = 11 };

enum {
  POINT = 0,
  LINE = 1,
  ARC = 2,
  ELLIPSE = 3,
  SPLINE = 4,
  PLANE = 5,
  RULED = 6,
  VOLUME = 7,
  LOOP = 8,
  SHELL = 9,
  GROUP = 10
};

extern char const* const type_names[NTYPES];
extern char const* const physical_type_names[NTYPES];
extern int const type_dims[NTYPES];

int is_entity(int t);
int is_face(int t);
int is_boundary(int t);

int get_boundary_type(int cell_type);

enum {
  FORWARD = 0,
  REVERSE = 1,
};

struct Object;

typedef std::shared_ptr<Object> ObjPtr;

struct Use {
  int dir;
  ObjPtr obj;
};

struct Object {
  int type;
  int id;
  std::vector<Use> used;
  std::vector<ObjPtr> helpers;
  std::vector<ObjPtr> embedded;
  int scratch;
  Object(int type);
  virtual ~Object();
};

ObjPtr new_object(int type);

int get_used_dir(ObjPtr user, ObjPtr used);
std::vector<ObjPtr> get_objs_used(ObjPtr user);

void print_object(FILE* f, ObjPtr obj);
void print_object_physical(FILE* f, ObjPtr obj);
void print_closure(FILE* f, ObjPtr obj);
void print_simple_object(FILE* f, ObjPtr obj);

void write_closure_to_geo(ObjPtr obj, char const* filename);

void print_object_dmg(FILE* f, ObjPtr obj);
int count_of_type(std::vector<ObjPtr> const& objs, int type);
int count_of_dim(std::vector<ObjPtr> const& objs, int dim);
void print_closure_dmg(FILE* f, ObjPtr obj);

void write_closure_to_dmg(ObjPtr obj, char const* filename);

void add_use(ObjPtr by, int dir, ObjPtr of);
void add_helper(ObjPtr to, ObjPtr h);
std::vector<ObjPtr> get_closure(ObjPtr obj, bool include_helpers,
    bool include_embedded = false);
std::vector<ObjPtr> filter_by_dim(std::vector<ObjPtr> const& objs, int dim);

struct Vector {
  double x, y, z;
};
struct Matrix {
  Vector x, y, z;
}; /* columns, not rows ! */

static inline Vector add_vectors(Vector a, Vector b) {
  return Vector{a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vector subtract_vectors(Vector a, Vector b) {
  return Vector{a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vector scale_vector(double a, Vector b) {
  return Vector{a * b.x, a * b.y, a * b.z};
}

static inline double dot_product(Vector a, Vector b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline double vector_norm(Vector a) { return sqrt(dot_product(a, a)); }

static inline Vector normalize_vector(Vector v) {
  return scale_vector(1.0 / vector_norm(v), v);
}

static inline Vector matrix_vector_product(Matrix a, Vector b) {
  return add_vectors(
      scale_vector(b.x, a.x),
      add_vectors(scale_vector(b.y, a.y), scale_vector(b.z, a.z)));
}

static inline Matrix cross_product_matrix(Vector a) {
  return Matrix{Vector{0, a.z, -a.y}, Vector{-a.z, 0, a.x},
                Vector{a.y, -a.x, 0}};
}

static inline Vector cross_product(Vector a, Vector b) {
  return matrix_vector_product(cross_product_matrix(a), b);
}

static inline Matrix tensor_product_matrix(Vector a, Vector b) {
  return Matrix{scale_vector(b.x, a), scale_vector(b.y, a),
                scale_vector(b.z, a)};
}

static inline Matrix identity_matrix() {
  return Matrix{Vector{1, 0, 0}, Vector{0, 1, 0}, Vector{0, 0, 1}};
}

static inline Matrix scale_matrix(double a, Matrix b) {
  return Matrix{scale_vector(a, b.x), scale_vector(a, b.y),
                scale_vector(a, b.z)};
}

static inline Matrix add_matrices(Matrix a, Matrix b) {
  return Matrix{add_vectors(a.x, b.x), add_vectors(a.y, b.y),
                add_vectors(a.z, b.z)};
}

static inline Matrix rotation_matrix(Vector axis, double angle) {
  return add_matrices(
      scale_matrix(cos(angle), identity_matrix()),
      add_matrices(
          scale_matrix(sin(angle), cross_product_matrix(axis)),
          scale_matrix(1 - cos(angle), tensor_product_matrix(axis, axis))));
}

static inline Vector rotate_vector(Vector axis, double angle, Vector v) {
  return matrix_vector_product(rotation_matrix(axis, angle), v);
}

struct Point : public Object {
  Vector pos;
  double size;
  Point();
  ~Point();
};

typedef std::shared_ptr<Point> PointPtr;

extern double default_size;

PointPtr new_point();
PointPtr new_point2(Vector v);
PointPtr new_point3(Vector v, double size);
std::vector<PointPtr> new_points(std::vector<Vector> vs);
std::vector<PointPtr> filter_points(std::vector<ObjPtr> const& objs);

void print_point(FILE* f, PointPtr p);

struct Extruded {
  ObjPtr middle;
  ObjPtr end;
};

typedef std::function<Vector(Vector)> Transform;

Extruded extrude_point(PointPtr start, Vector v);
Extruded extrude_point2(PointPtr start, Transform tr);
std::vector<Extruded> extrude_points(std::vector<PointPtr> const& points,
    Transform tr);
PointPtr edge_point(ObjPtr edge, int i);

ObjPtr new_line();
ObjPtr new_line2(PointPtr start, PointPtr end);
ObjPtr new_line3(Vector origin, Vector span);
ObjPtr new_line4(Vector a, Vector b);

ObjPtr new_arc();
ObjPtr new_arc2(PointPtr start, PointPtr center, PointPtr end);
PointPtr arc_center(ObjPtr arc);
Vector arc_normal(ObjPtr arc);
void print_arc(FILE* f, ObjPtr arc);

ObjPtr new_ellipse();
ObjPtr new_ellipse2(PointPtr start, PointPtr center, PointPtr major_pt,
                    PointPtr end);
PointPtr ellipse_center(ObjPtr e);
PointPtr ellipse_major_pt(ObjPtr e);
void print_ellipse(FILE* f, ObjPtr e);

ObjPtr new_spline();
ObjPtr new_spline2(std::vector<PointPtr> const& pts);
ObjPtr new_spline3(std::vector<Vector> const& pts);
void print_spline(FILE* f, ObjPtr e);

Extruded extrude_edge(ObjPtr start, Vector v);
Extruded extrude_edge2(ObjPtr start, Vector v, Extruded left, Extruded right);
Extruded extrude_edge3(ObjPtr start, Transform tr, Extruded left, Extruded right);
std::vector<Extruded> extrude_edges(std::vector<ObjPtr> const& edges,
    Transform tr, std::vector<Extruded> const& point_extrusions);

ObjPtr new_loop();
std::vector<PointPtr> loop_points(ObjPtr loop);
Extruded extrude_loop(ObjPtr start, Vector v);
Extruded extrude_loop2(ObjPtr start, Vector v, ObjPtr shell, int shell_dir);
Extruded extrude_loop3(ObjPtr start, Transform tr, ObjPtr shell, int shell_dir);
Extruded extrude_loop4(ObjPtr start, ObjPtr shell, int shell_dir,
    std::vector<Extruded> const& edge_extrusions);

ObjPtr new_circle(Vector center, Vector normal, Vector x);
ObjPtr new_ellipse3(Vector center, Vector major, Vector minor);
ObjPtr new_polyline(std::vector<PointPtr> const& pts);
ObjPtr new_polyline2(std::vector<Vector> const& vs);

ObjPtr new_plane();
ObjPtr new_plane2(ObjPtr loop);
Vector plane_normal(ObjPtr plane, double epsilon = 1e-10);

ObjPtr new_square(Vector origin, Vector x, Vector y);
ObjPtr new_disk(Vector center, Vector normal, Vector x);
ObjPtr new_elliptical_disk(Vector center, Vector major, Vector minor);
ObjPtr new_polygon(std::vector<Vector> const& vs);

ObjPtr new_ruled();
ObjPtr new_ruled2(ObjPtr loop);

void add_hole_to_face(ObjPtr face, ObjPtr loop);
Extruded extrude_face(ObjPtr face, Vector v);
Extruded extrude_face2(ObjPtr face, Transform tr);
Extruded extrude_face3(ObjPtr face, std::vector<Extruded> const& edge_extrusions);
Extruded extrude_face_group(ObjPtr face_group, Transform tr);
ObjPtr face_loop(ObjPtr face);

ObjPtr new_shell();

void make_hemisphere(ObjPtr circle, PointPtr center, ObjPtr shell, int dir);
ObjPtr new_sphere(Vector center, Vector normal, Vector x);

ObjPtr new_volume();
ObjPtr new_volume2(ObjPtr shell);
ObjPtr volume_shell(ObjPtr v);

ObjPtr new_cube(Vector origin, Vector x, Vector y, Vector z);

enum cube_face { BOTTOM, TOP, FRONT, RIGHT, BACK, LEFT };

ObjPtr get_cube_face(ObjPtr cube, enum cube_face which);

ObjPtr new_ball(Vector center, Vector normal, Vector x);

void insert_into(ObjPtr into, ObjPtr f);

ObjPtr new_group();
void add_to_group(ObjPtr group, ObjPtr o);

void weld_volume_face_into(ObjPtr big_volume, ObjPtr small_volume,
                           ObjPtr big_volume_face, ObjPtr small_volume_face);
void weld_plane_with_holes_into(ObjPtr big_volume, ObjPtr small_volume,
                           ObjPtr big_volume_face, ObjPtr small_volume_face);

Vector eval(ObjPtr o, double const* param);

void transform_closure(ObjPtr object, Matrix linear, Vector translation);

ObjPtr copy_closure(ObjPtr object);

ObjPtr collect_assembly_boundary(ObjPtr assembly);

void unscramble_loop(ObjPtr loop);

void weld_half_shell_onto(ObjPtr volume, ObjPtr big_face,
    ObjPtr half_shell, int dir);

void embed(ObjPtr into, ObjPtr embedded);

}  // end namespace gmod

static inline gmod::Vector operator+(gmod::Vector a, gmod::Vector b) {
  return gmod::add_vectors(a, b);
}

static inline gmod::Vector operator-(gmod::Vector a, gmod::Vector b) {
  return gmod::subtract_vectors(a, b);
}

static inline gmod::Vector operator*(double a, gmod::Vector b) {
  return gmod::scale_vector(a, b);
}

static inline gmod::Vector operator/(gmod::Vector a, double b) {
  return gmod::scale_vector(1.0 / b, a);
}

static inline gmod::Vector operator*(gmod::Matrix a, gmod::Vector b) {
  return gmod::matrix_vector_product(a, b);
}

#endif
