#include <chrono>
#include <algorithm>
#include <tuple>
#include "Glsl.hpp"

namespace glsl {
     vec3 vec3::operator*(float n){
          return {x*n,y*n,z*n};
     }

     vec3 vec3::operator*(vec3 v){
          return {x*v.x,y*v.y,z*v.z};
     }

     vec3 vec3::operator-(vec3 v){
          return {x-v.x,y-v.y,z-v.z};
     }

     vec3 vec3::operator-(float v){
          return {x-v,y-v,z-v};
     }

     vec3 vec3::operator+(vec3 v){
          return {x+v.x,y+v.y,z+v.z};
     }

     vec3 vec3::operator+(float v){
          return {x+v,y+v,z+v};
     }

     vec3& vec3::operator*=(vec3 v){
          x*=v.x;y*=v.y;z*=v.z;
          return *this;
     }

     bool vec3::operator<(vec3 v) const {
          return std::tuple(x,y,z) < std::tuple(v.x, v.y,v.z);
     }

     vec2 vec2::operator-(vec2 v) {
          return vec2{x-v.x,y-v.y};
     }
     
     vec2 vec2::operator-(float v) {
          return vec2{x-v,y-v};
     }

     vec2 vec2::operator+(float v) {
          return vec2{x+v,y+v};
     }

     vec2 vec2::operator*(float v) {
          return vec2{x * v, y * v};
     }

     vec2 vec2::operator/(float v) {
           return vec2{x/v, y/v};
     }

     float vec3::length() const {
          return std::sqrtf(x*x + y*y + z*z);
     }

     float smoothstep(float edge0, float edge1, float x) {
          float t = std::clamp((x-edge0)/(edge1-edge0), 0.0f, 1.0f);
          return t * t * (3.0f-2.0f*t);
     }

     float get_i_time(){
          static auto start_time = std::chrono::steady_clock::now();
          return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now()-start_time).count();
     }

     float vec2::length() const {
          return std::sqrtf(x*x + y*y);
     }

     unsigned char color_to_byte(float c) {
          return std::floor(c * 255.0 + 0.5);
     }
     float dot_vec3(vec3 a, vec3 b) {
          return a.x * b.x + a.y * b.y + a.z * b.z;
     }

     vec3 mul_mat3(const mat3& m, vec3 v){
          auto x = m.m[0][0] * v.x + m.m[0][1]*v.y +  m.m[0][2]*v.z;
          auto y = m.m[1][0] * v.x + m.m[1][1]*v.y +  m.m[1][2]*v.z;
          auto z = m.m[2][0] * v.x + m.m[2][1]*v.y +  m.m[2][2]*v.z;
          return {x,y,z};
     }

     float mod(float x, float y) {
          float res = std::fmod(x,y);
          if (res<0.0f) res+=y;
          return res;
     }
     vec3 max_vec3(vec3 a, vec3 b){
          return {
               std::max(a.x, b.x),
               std::max(a.y, b.y),
               std::max(a.z, b.z),
          };
     }
     vec3 mode_vec3(vec3 vec, float n){
          return vec3{mod(vec.x,n),mod(vec.y,n),mod(vec.z,n)};
     }
}