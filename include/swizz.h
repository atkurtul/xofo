
#ifndef C62CF1EB_3F09_4301_8CCF_41B0C0FFBE6E
#define C62CF1EB_3F09_4301_8CCF_41B0C0FFBE6E

#include <math.h>
#include <algorithm>
#include <cstring>
#include <type_traits>

#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

namespace xofo {
#define swizz_name_0 x
#define swizz_name_1 y
#define swizz_name_2 z
#define swizz_name_3 w
#define swizz_name_
#define swizz_name(n) swizz_name_##n

#define concat_names(x, y, z, w) x##y##z##w

#define expand(do_, ...) do_(__VA_ARGS__)

// clang-format off

#define swizz_name_full(x,y,z,w)   expand(concat_names, swizz_name(x), swizz_name(y), swizz_name(z), swizz_name(w))


#define define_swizz_name_4(n, x,y,z,w)   swizz<n, T, x,y,z,w>  swizz_name_full(x,y,z,w);
#define define_swizz_name_3(n, x,y,z)     swizz<n, T, x,y,z>    swizz_name_full(x,y,z,);
#define define_swizz_name_2(n, x,y)       swizz<n, T, x,y>      swizz_name_full(x,y,,);
#define define_scalar(n, x)               swizz<n, T, x>        swizz_name(x);

#define define_swizz4_w(x,y,z,w)     define_swizz_name_4(4, x,y,z,w)
#define define_swizz4_z(x,y,z)       define_swizz_name_3(4, x,y,z)  define_swizz4_w(x,y,z,0)   define_swizz4_w(x,y,z,1)   define_swizz4_w(x,y,z,2)   define_swizz4_w(x,y,z,3)
#define define_swizz4_y(x,y)         define_swizz_name_2(4, x,y)    define_swizz4_z(x,y,0)     define_swizz4_z(x,y,1)     define_swizz4_z(x,y,2)     define_swizz4_z(x,y,3) 
#define define_swizz4_x(x)           define_swizz4_y(x,0)           define_swizz4_y(x,1)       define_swizz4_y(x,2)       define_swizz4_y(x,3) 
#define define_swizz4                define_swizz4_x(0)             define_swizz4_x(1)         define_swizz4_x(2)         define_swizz4_x(3)

#define define_swizz3_w(x,y,z,w)     define_swizz_name_4(3, x,y,z,w)
#define define_swizz3_z(x,y,z)       define_swizz_name_3(3, x,y,z)  define_swizz3_w(x,y,z,0)   define_swizz3_w(x,y,z,1)   define_swizz3_w(x,y,z,2)
#define define_swizz3_y(x,y)         define_swizz_name_2(3, x,y)    define_swizz3_z(x,y,0)     define_swizz3_z(x,y,1)     define_swizz3_z(x,y,2)
#define define_swizz3_x(x)           define_swizz3_y(x,0)           define_swizz3_y(x,1)       define_swizz3_y(x,2)
#define define_swizz3                define_swizz3_x(0)             define_swizz3_x(1)         define_swizz3_x(2)

#define define_swizz2_w(x,y,z,w)     define_swizz_name_4(2, x,y,z,w)
#define define_swizz2_z(x,y,z)       define_swizz_name_3(2, x,y,z)  define_swizz2_w(x,y,z,0)   define_swizz2_w(x,y,z,1)
#define define_swizz2_y(x,y)         define_swizz_name_2(2, x,y)    define_swizz2_z(x,y,0)     define_swizz2_z(x,y,1)
#define define_swizz2_x(x)           define_swizz2_y(x,0)           define_swizz2_y(x,1)
#define define_swizz2                define_swizz2_x(0)             define_swizz2_x(1)

// clang-format on

#define INLINE inline constexpr __attribute__((always_inline))

template <class T, class U>
static constexpr bool same_type = false;

template <class T>
static constexpr bool same_type<T, T> = true;

template <u32 m, class T, u32... n>
requires((n < m) && ...) struct swizz;

template <u32... i>
struct actual_sequence;
template <u32 i, u32... rest>
struct sequence_maker : sequence_maker<i - 1, i, rest...> {};

template <u32... i>
struct sequence_maker<0, i...> {
  using type = actual_sequence<0, i...>;

  template <u32 m, class T, u32 scale, u32 offset = 0>
  using col_t = swizz<m, T, offset, offset + i * scale...>;
};

template <u32 r, u32 m, class T, u32 scale, u32 offset = 0>
using col_t =
    typename sequence_maker<r - 1>::template col_t<m, T, scale, offset>;

template <class T, u32 n>
struct vector_base;

template <class T, u32 R, u32 C>
struct matrix_base;

template <u32... i>
struct actual_sequence {
  static constexpr u32 R = sizeof...(i);

  template <class T>
  struct functs {
    using Func = T(T, T);
    using vect = vector_base<T, R>;

    template <u32 C>
    struct mat_functs {
      using Func = vect(vect, vect);

      static INLINE vector_base<T, C> mul_vm(vector_base<T, R> const& l,
                                             matrix_base<T, R, C> const& r) {
        return ((l.v[i] * r.row[i]) + ...);
      }

      static INLINE vector_base<T, R> mul_mv(matrix_base<T, R, C> const& l,
                                             vector_base<T, C> const& r) {
        return vector_base<T, R>(dot(l.row[i], r)...);
      }

      template <u32 O>
      static INLINE matrix_base<T, R, O> mul_mm(matrix_base<T, R, C> const& l,
                                                matrix_base<T, C, O> const& r) {
        return matrix_base<T, R, O>(l.row[i] * r...);
      }

      static INLINE matrix_base<T, R, C> transpose(
          matrix_base<T, C, R> const& m) {
        return matrix_base<T, R, C>(((col_t<R, R * C, T, C, i>&)m)...);
      }
    };

    static INLINE vect foreach (vect l, vect r, Func f) {
#if NDEBUG
      if constexpr (R == 3)
        return actual_sequence<i..., 4>::template functs<T>::foreach (l, r, f);
#endif
      return vect(f(l.v[i], r.v[i])...);
    }

    static INLINE vect map(vect v, T (*f)(T)) { return vect(f(v.v[i])...); }

    static INLINE T hsum(vect v) { return (v[i] + ...); }
  };
};

template <class T, u32 i>
using sequence = typename sequence_maker<i - 1>::type::template functs<T>;

template <u32 m, class T, u32... n>
requires((n < m) && ...) struct swizz {
  using vect = vector_base<T, sizeof...(n)>;
  INLINE operator vect() const { return vect(v[n]...); }

#define define_op(op)                                                      \
  INLINE vect operator op(vect r) const { return vect(*this) op r; }       \
  friend INLINE vect operator op(vect l, swizz r) { return l op vect(r); } \
  template <u32 lenr, u32... nr>                                           \
  INLINE vect operator op(swizz<lenr, T, nr...> r) const {                 \
    return vect(*this) op r;                                               \
  }

  define_op(*) define_op(/) define_op(+) define_op(-);
#undef define_op

  INLINE vect operator-() const { return -vect(*this); }

  INLINE T operator[](u32 i) const { return v[idx[i]]; }
  INLINE T& operator[](u32 i) { return v[idx[i]]; }

  INLINE auto norm() const { return norm(*this); }
  INLINE auto len() const { return len(*this); }

 private:
  static constexpr u32 idx[] = {n...};
  T v[m];
};

template <class T, int n>
constexpr u32 alignment() {
  u32 size = sizeof(T) * n;
  return (size & (size - 1)) ? 1 : size;
}

template <class T>
struct __attribute__((aligned(alignment<T, 4>()))) vector_base<T, 4> {
  union {
    T v[4];
    struct {
      T x, y, z, w;
    };
    define_swizz4;
  };

  INLINE vector_base() : v{} {};
  INLINE vector_base(T s) : v{s, s, s, s} {};
  INLINE vector_base(T x, T y, T z, T w) : v{x, y, z, w} {};
  INLINE vector_base(vector_base const& v) : v{v.x, v.y, v.z, v.w} {};

  INLINE vector_base(vector_base<T, 3> const& v, T w) : v{v.x, v.y, v.z, w} {};
  INLINE vector_base(T x, vector_base<T, 3> const& v) : v{x, v.x, v.y, v.z} {};

  INLINE vector_base(vector_base<T, 2> const& v, T z, T w)
      : v{v.x, v.y, z, w} {};
  INLINE vector_base(T x, vector_base<T, 2> const& v, T w)
      : v{x, v.x, v.y, w} {};
  INLINE vector_base(T x, T y, vector_base<T, 2> const& v)
      : v{x, y, v.x, v.y} {};

  INLINE T operator[](u32 i) const { return v[i]; }
  INLINE T& operator[](u32 i) { return v[i]; }

  INLINE auto norm() const { return norm(*this); }
  INLINE auto len() const { return len(*this); }
};

template <class T>
struct __attribute__((aligned(alignment<T, 3>()))) vector_base<T, 3> {
  union {
    T v[3];
    struct {
      T x, y, z;
    };
    define_swizz3;
  };

  INLINE operator vector_base<T, 4>() const {
    return vector_base<T, 4>(*this, T(0));
  }
  INLINE vector_base(vector_base<T, 4> const& v) : v{v.x, v.y, v.z} {}

  INLINE vector_base() : v{} {};
  INLINE vector_base(T s) : v{s, s, s} {};
  INLINE vector_base(T x, T y, T z) : v{x, y, z} {};
  INLINE vector_base(vector_base const& v) : v{v.x, v.y, v.z} {};

  INLINE vector_base(vector_base<T, 2> const& v, T z) : v{v.x, v.y, z} {};
  INLINE vector_base(T x, vector_base<T, 2> const& v) : v{x, v.x, v.y} {};

  INLINE T operator[](u32 i) const { return v[i]; }
  INLINE T& operator[](u32 i) { return v[i]; }

  INLINE auto norm() const { return norm(*this); }
  INLINE auto len() const { return len(*this); }
};

template <class T>
struct __attribute__((aligned(alignment<T, 2>()))) vector_base<T, 2> {
  union {
    T v[2];
    struct {
      T x, y;
    };
    define_swizz2;
  };

  INLINE vector_base() : v{} {};
  INLINE vector_base(T s) : v{s, s} {};
  INLINE vector_base(T x, T y) : v{x, y} {};
  INLINE vector_base(vector_base const& v) : v{v.x, v.y} {};
  INLINE T operator[](u32 i) const { return v[i]; }
  INLINE T& operator[](u32 i) { return v[i]; }

  INLINE auto norm() const { return norm(*this); }
  INLINE auto len() const { return len(*this); }
};

template <class T, u32 n>
struct __attribute__((aligned(alignment<T, n>()))) vector_base {
  T v[n];
  INLINE vector_base() : v{} {};
  INLINE vector_base(T s) : v{s, s} {};
  template <class... S>
  requires(sizeof...(S) <= n && (same_type<S, T> && ...)) INLINE
      vector_base(S... args)
      : v{args...} {};
  INLINE T operator[](u32 i) const { return v[i]; }
  INLINE T& operator[](u32 i) { return v[i]; }

  INLINE auto norm() const { return norm(*this); }
  INLINE auto len() const { return len(*this); }
};

template <class T, u32 n>
INLINE vector_base<T, n> doop(vector_base<T, n> l,
                              vector_base<T, n> r,
                              T (*f)(T, T)) {
  return sequence<T, n>::foreach (l, r, f);
}

#define define_op0(op, L, R)                                  \
  template <class T, u32 n>                                   \
  INLINE vector_base<T, n> operator op(L l, R r) {            \
    return doop<T, n>(l, r, [](T a, T b) { return a op b; }); \
  }

#define vect vector_base<T, n>
#define define_op(op) \
  define_op0(op, T, vect) define_op0(op, vect, T) define_op0(op, vect, vect)

define_op(*) define_op(+) define_op(-) define_op(/);

#undef vect
#undef define_op0
#undef define_op

template <class T, u32 n>
INLINE vector_base<T, n> operator-(vector_base<T, n> v) {
  return sequence<T, n>::map(v, [](T s) { return -s; });
}

#define overload_binop(func, ret)                             \
  template <class T, u32 n, u32 s, u32... m>                  \
  INLINE ret func(vector_base<T, n> l, swizz<s, T, m...> r) { \
    return func(l, vector_base<T, n>(r));                     \
  }                                                           \
  template <class T, u32 n, u32 s, u32... m>                  \
  INLINE ret func(swizz<s, T, m...> l, vector_base<T, n> r) { \
    return func(vector_base<T, n>(l), r);                     \
  }                                                           \
  template <class T, u32 n>                                   \
  INLINE ret func(vector_base<T, n> l, T r) {                 \
    return func(l, vector_base<T, n>(r));                     \
  }                                                           \
  template <class T, u32 n>                                   \
  INLINE ret func(T l, vector_base<T, n> r) {                 \
    return func(vector_base<T, n>(l), r);                     \
  }

#define overload_unop(func, ret)         \
  template <u32 n, class T, u32... m>    \
  INLINE ret func(swizz<n, T, m...> v) { \
    return func(v);                      \
  }

template <class T, u32 n>
INLINE T dot(vector_base<T, n> l, vector_base<T, n> r) {
  return sequence<T, n>::hsum(l * r);
}

template <class T, u32 n>
INLINE T len(vector_base<T, n> v) {
  return sqrt(dot(v, v));
}

template <class T, u32 n>
INLINE vector_base<T, n> norm(vector_base<T, n> v) {
  return v / sqrt(dot(v, v));
}

template <class T>
INLINE T min(T a, T b) {
  return a < b ? a : b;
}

template <class T>
INLINE T max(T a, T b) {
  return a > b ? a : b;
}

template <class T>
INLINE T abs(T a) {
  return a < 0 ? -a : a;
}

template <class T, u32 n>
INLINE vector_base<T, n> min(vector_base<T, n> l, vector_base<T, n> r) {
  return sequence<T, n>::foreach (l, r, min);
}

template <class T, u32 n>
INLINE vector_base<T, n> max(vector_base<T, n> l, vector_base<T, n> r) {
  return sequence<T, n>::foreach (l, r, max);
}

template <class T, u32 n>
INLINE vector_base<T, n> abs(vector_base<T, n> l) {
  return sequence<T, n>::map(l, abs);
}

INLINE auto clamp(auto v, auto lo, auto hi) {
  using T = decltype(v * hi * lo);
  auto mx = max(T(lo), T(v));
  return min(T(hi), T(mx));
}

template <class T>
INLINE T saturate(T v) {
  return clamp(v, T(0), T(1));
}

INLINE auto smoothstep(auto a, auto b, auto t) {
  auto s = saturate((t - a) / (b - a));
  using S = decltype(s);
  return s * s * (S(3) - S(2) * s);
}

INLINE auto lerp(auto a, auto b, auto t) {
  return a + (b - a) * t;
}

overload_binop(dot, T);
overload_binop(min, T);
overload_binop(max, T);
overload_unop(len, T);
overload_unop(norm, T);
overload_unop(abs, T);

#undef overload_binop
#undef overload_unop

#undef swizz_name_0
#undef swizz_name_1
#undef swizz_name_2
#undef swizz_name_3
#undef swizz_name_
#undef swizz_name
#undef concat_names
#undef eval_names
#undef swizz_name_full
#undef define_swizz_w
#undef define_swizz_z
#undef define_swizz_y
#undef define_swizz_x
#undef define_swizz

#define typedef_vec(n)                 \
  typedef vector_base<i8, n> i8x##n;   \
  typedef vector_base<u8, n> u8x##n;   \
  typedef vector_base<i16, n> i16x##n; \
  typedef vector_base<u16, n> u16x##n; \
  typedef vector_base<i32, n> i32x##n; \
  typedef vector_base<u32, n> u32x##n; \
  typedef vector_base<i64, n> i64x##n; \
  typedef vector_base<u64, n> u64x##n; \
  typedef vector_base<f32, n> f32x##n; \
  typedef vector_base<f64, n> f64x##n;

typedef_vec(2);
typedef_vec(3);
typedef_vec(4);
typedef_vec(5);
typedef_vec(6);
typedef_vec(7);
typedef_vec(8);
typedef_vec(9);
typedef_vec(10);
typedef_vec(11);
typedef_vec(12);
typedef_vec(13);
typedef_vec(14);
typedef_vec(15);
typedef_vec(16);

#undef typedef_vec

template <class T, u32 R, u32 C>
struct matrix_base {
  template <u32 n>
  using colt = col_t<R, R * C, T, C, n>;

  using sequence = typename sequence<T, R>::template mat_functs<C>;

  union {
    T data[R][C];
    vector_base<T, C> row[R];
  };

  INLINE matrix_base(T s = T(1)) : data{0} {
    for (u32 i = 0; i < min(R, C); ++i)
      data[i][i] = s;

    if constexpr (R == C) {
      data[R - 1][C - 1] = T(1);
    }
  }

  template <class... Args>
  requires(sizeof...(Args) == R &&
           ((std::is_convertible_v<Args, vector_base<T, C>>)&&...)) INLINE
      matrix_base(Args... s)
      : row{s...} {}

  matrix_base(matrix_base const& m) { memcpy(this, &m, sizeof(*this)); }

  INLINE vector_base<T, C> operator[](u32 i) const { return row[i]; }

  INLINE vector_base<T, C>& operator[](u32 i) { return row[i]; }

  template <u32 n>
  INLINE colt<n> const& col() const {
    return (colt<n>&)*this;
  }

  INLINE friend vector_base<T, C> operator*(vector_base<T, R> const& v,
                                            matrix_base const& m) {
    return sequence::mul_vm(v, m);
  }

  INLINE vector_base<T, R> operator*(vector_base<T, C> const& v) {
    return sequence::mul_mv(*this, v);
  }

  template <u32 O>
  INLINE matrix_base<T, R, O> operator*(matrix_base<T, C, O> const& r) {
    return sequence::mul_mm(*this, r);
  }
};

template <class T, u32 R, u32 C>
INLINE matrix_base<T, C, R> transpose(matrix_base<T, R, C> const& m) {
  return matrix_base<T, C, R>::sequence::transpose(m);
}

template <class T>
INLINE vector_base<T, 3> cross(auto a, auto b) {
  vector_base<T, 3> l = a;
  vector_base<T, 3> r = b;
  return l.yzx * r.zxy - l.zxy * r.yzx;
}

template <class T>
INLINE T determinant(matrix_base<T, 2, 2> const& m) {
  return cross(m[0], m[1]);
}

template <class T>
INLINE T tproduct(auto x, auto y, auto z) {
  return dot(x, cross<T>(y, z));
}

template <class T>
INLINE T determinant(matrix_base<T, 3, 3> const& m) {
  return tproduct(m[0], m[1], m[2]);
}

template <class T>
INLINE vector_base<T, 4> cross(auto a, auto b, auto c) {
  vector_base<T, 4> x = a;
  vector_base<T, 4> y = b;
  vector_base<T, 4> z = c;
  return vector_base<T, 4>(
      tproduct<T>(x.yzw, y.yzw, z.yzw), -tproduct<T>(x.xzw, y.xzw, z.xzw),
      tproduct<T>(x.xyw, y.xyw, z.xyw), -tproduct<T>(x.xyz, y.xyz, z.xyz));
}

template <class T>
INLINE T determinant(matrix_base<T, 4, 4> const& m) {
  return dot(m[0], cross(m[1], m[2], m[3]));
}

#define typedef_mat(r, c)                       \
  typedef matrix_base<i8, r, c> i8x##r##x##c;   \
  typedef matrix_base<u8, r, c> u8x##r##x##c;   \
  typedef matrix_base<i16, r, c> i16x##r##x##c; \
  typedef matrix_base<u16, r, c> u16x##r##x##c; \
  typedef matrix_base<i32, r, c> i32x##r##x##c; \
  typedef matrix_base<u32, r, c> u32x##r##x##c; \
  typedef matrix_base<i64, r, c> i64x##r##x##c; \
  typedef matrix_base<u64, r, c> u64x##r##x##c; \
  typedef matrix_base<f32, r, c> f32x##r##x##c; \
  typedef matrix_base<f64, r, c> f64x##r##x##c;

#define typedef_matr(c) \
  typedef_mat(2, c);    \
  typedef_mat(3, c);    \
  typedef_mat(4, c);    \
  typedef_mat(5, c);    \
  typedef_mat(6, c);    \
  typedef_mat(7, c);    \
  typedef_mat(8, c);

typedef_matr(2);
typedef_matr(3);
typedef_matr(4);
typedef_matr(5);
typedef_matr(6);
typedef_matr(7);
typedef_matr(8);

template <class T>
INLINE matrix_base<T, 4, 4> translate(matrix_base<T, 4, 4> const& m,
                                      auto offset) {
  const auto v = vector_base<T, 4>(vector_base<T, 3>(offset), 0);

  return matrix_base<T, 4, 4>(m[0].w * v + m[0], m[1].w * v + m[1],
                              m[2].w * v + m[2], m[3].w * v + m[3]);
}

template <class T>
INLINE matrix_base<T, 4, 4> axis_angle(auto ax, T angle) {
  vector_base<T, 3> axis = ax;
  if (dot(axis, axis) < std::numeric_limits<float>::epsilon()) {
    return matrix_base<T, 4, 4>(1);
  }

  auto n = norm(axis);

  const T c = std::cos(angle);
  const T vs = 1 - c;

  auto ns = n * std::sin(angle);
  auto d = n * n * vs + c;
  auto cx = n.xyz * n.yzx * vs;

  using vec4 = vector_base<T, 4>;

  return matrix_base<T, 4, 4>(vec4(d.x, cx.x + ns.z, cx.z - ns.y, 0),
                              vec4(cx.x - ns.z, d.y, cx.y + ns.x, 0),
                              vec4(cx.z + ns.y, cx.y - ns.x, d.z, 0),
                              vec4(0, 0, 0, 1));
}

template <class T>
INLINE matrix_base<T, 4, 4> ortho(T left,
                                  T right,
                                  T bottom,
                                  T top,
                                  T znear,
                                  T zfar) {
  T x = T(2) / (right - left);
  T y = T(2) / (bottom - top);
  T z = T(1) / (znear - zfar);
  T a = (right + left) / (left - right);
  T b = (top + bottom) / (top - bottom);
  T c = (zfar + znear) / (zfar - znear) * -T(0.5);

  using vec4 = vector_base<T, 4>;
  return matrix_base<T, 4, 4>(vec4(x, 0, 0, 0), vec4(0, y, 0, 0),
                              vec4(0, 0, z, z), vec4(a, b, c, c + T(1)));
}

template <class T>
INLINE matrix_base<T, 4, 4> frustum(T left,
                                    T right,
                                    T bottom,
                                    T top,
                                    T znear,
                                    T zfar) {
  T a = (right + left) / (right - left);
  T b = (top + bottom) / (bottom - top);
  T c = (zfar + znear) / (znear - zfar) * T(0.5);
  T d = (T(2) * znear * zfar) / (znear - zfar) * T(0.5);
  T x = (T(2) * znear) / (right - left);
  T y = (T(2) * znear) / (bottom - top);

  using vec4 = vector_base<T, 4>;

  return matrix_base<T, 4, 4>(vec4(x, 0, 0, 0), vec4(0, y, 0, 0),
                              vec4(a, b, c, c - T(1)), vec4(0, 0, d, d));
}

template <class T>
INLINE matrix_base<T, 4, 4> perspective(T x, T y, T n, T f) {
  x = n * std::tan(x * T(0.5));
  y = n * std::tan(y * T(0.5));
  return frustum(-x, x, -y, y, n, f);
}

// INLINE f32x4x4 axis_angle(f32x3 axis, f32 angle) {
//   return impl::axis_angle(axis, angle);
// }

// INLINE f64x4x4 axis_angle(f64x3 axis, f64 angle) {
//   return impl::axis_angle(axis, angle);
// }

// INLINE f32x4x4
// ortho(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar) {
//   return impl::ortho(left, right, bottom, top, znear, zfar);
// }

// INLINE f64x4x4
// ortho(f64 left, f64 right, f64 bottom, f64 top, f64 znear, f64 zfar) {
//   return impl::ortho(left, right, bottom, top, znear, zfar);
// }

// INLINE f32x4x4
// frustum(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar) {
//   return impl::frustum(left, right, bottom, top, znear, zfar);
// }

// INLINE f64x4x4
// frustum(f64 left, f64 right, f64 bottom, f64 top, f64 znear, f64 zfar) {
//   return impl::frustum(left, right, bottom, top, znear, zfar);
// }

// INLINE f32x4x4 perspective(f32 x, f32 y, f32 n, f32 f) {
//   return impl::perspective(x, y, n, f);
// }

// INLINE f64x4x4 perspective(f64 x, f64 y, f64 n, f64 f) {
//   return impl::perspective(x, y, n, f);
// }

template <class T>
INLINE matrix_base<T, 4, 4> inverse(matrix_base<T, 4, 4> const& m) {
  using mat4 = matrix_base<T, 4, 4>;

  auto col0 = cross<T>(m[1], m[2], m[3]);

  auto inv_det = T(1) / dot(m[0], col0);

  auto col1 = -cross<T>(m[0], m[2], m[3]) * inv_det;
  auto col2 =  cross<T>(m[0], m[1], m[3]) * inv_det;
  auto col3 = -cross<T>(m[0], m[1], m[2]) * inv_det;

  return transpose(matrix_base<T, 4, 4>(col0 * inv_det, col1, col2, col3));
}

#undef INLINE

}  // namespace xofo
#endif /* C62CF1EB_3F09_4301_8CCF_41B0C0FFBE6E */
