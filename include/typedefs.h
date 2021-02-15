#ifndef A85441C4_7ECF_4EE1_8160_B7BEA0D5A239
#define A85441C4_7ECF_4EE1_8160_B7BEA0D5A239

#include <stdint.h>

#include <mango/math/matrix_float4x4.hpp>
#include <mango/math/math.hpp>
#include <mango/math/quaternion.hpp>
#include <mango/math/geometry.hpp>

#include <mango/image/surface.hpp>

#define RADIAN 0.01745329251f

using mat = mango::float4x4;
using vec2 = mango::float32x2;
using vec3 = mango::float32x3;
using vec4 = mango::float32x4;
using vec2d = mango::float64x2;
using vec3d = mango::float64x3;
using vec4d = mango::float64x4;
using vec2i = mango::int32x2;
using vec3i = mango::int32x3;
using vec4i = mango::int32x4;
using vec2u = mango::uint32x2;
using vec3u = mango::uint32x3;
using vec4u = mango::uint32x4;
using angax = mango::AngleAxis;

using mango::transpose;
using mango::vulkan::perspective;

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

#endif /* A85441C4_7ECF_4EE1_8160_B7BEA0D5A239 */
