#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include "Glsl.hpp"
#include <thread>
#include <vector>
constexpr size_t TILE_X = 10;
constexpr size_t TILE_Y = 10;
constexpr size_t HEIGHT = TILE_Y * 50;
constexpr size_t WIDTH = TILE_X * 50;
constexpr size_t FRAMES = 40;

glsl::pixel blue_pulse_shader(glsl::vec2 fragCoord, float w, float h){
    auto uv = (fragCoord - glsl::vec2{0.5f * w, 0.5f * h})/h;
    float dist = uv.length();
    float pulse = 0.5f + 0.5f * std::sin(glsl::get_i_time() * 2.0f);
    float glow = 1.0f - glsl::smoothstep(0.0f, 1.0f + pulse * 0.3f, dist);
    auto color = glsl::vec3{0.2f, 0.4f, 1.0f} * glow;
    return glsl::pixel{glsl::color_to_byte(color.x),glsl::color_to_byte(color.y),glsl::color_to_byte(color.z)};
}

glsl::mat3 getRotYMat(float a){
   return glsl::mat3{.m={
      {std::cos(a),0,std::sin(a)},
      {0.f,1.f,0.f},
      {-std::sin(a),0,std::cos(a)}}};
}

// https://www.shadertoy.com/view/MtsGzB
// TODO: debug
glsl::pixel weird_fractal(glsl::vec2 fragCoord, float w, float h){
  auto s = glsl::vec2{w,h};
  // in original iDate.w 
  float t = glsl::get_i_time()*0.2f,c,d,m;
  auto tmp = ((fragCoord*2.0f)-s)/s.x;
  glsl::vec3 p = glsl::vec3{tmp.x,tmp.y,1.0f},r=p-p,q=r;
  p = glsl::mul_mat3(getRotYMat(-t), p);
  auto xx = (glsl::vec2{std::sin(t),std::cos(t)}*3.0f) + 10.f;
  q = q + glsl::vec3{xx.x, 0.f, xx.y};
  for (float i=1.0f; i>0.f; i-=0.01f) {
      c=d=0.f,m=1.f;
      for (int j=0;j<3;j++){
          r=glsl::max_vec3(r*=r*=r*=r=glsl::mode_vec3(q*m+1.f,2.f)-1.f,glsl::vec3{r.y, r.z, r.x}),
          d=std::max(d, (0.29f - r.length() * 0.6f) / m) * 0.8f;
          m *= 1.1f;
      }
      q =q+(p*d);
      c = i;
      if(d<1e-5) break;
  }
  float k = glsl::dot_vec3(r,r+0.15f);
  auto res = (glsl::vec3{1.0f,k,k/c}-0.8f);
  return glsl::pixel{
    glsl::color_to_byte(res.x), 
    glsl::color_to_byte(res.y), 
    glsl::color_to_byte(res.z)
  };
}

int main(){
    std::stringstream ss;
    std::stringstream filename;  
    for (int f = 0; f < FRAMES; ++f){
        ss << "P6\n";
        ss << HEIGHT << ' ' << WIDTH << '\n';
        ss << 255 << '\n';
        for (int i = 0; i < HEIGHT; ++i){
          for (int j = 0; j < WIDTH; ++j){
                auto pixel = weird_fractal(glsl::vec2{float(j),float(HEIGHT-1-i)},WIDTH, HEIGHT);
                ss.write((char*)&pixel.r,1);
                ss.write((char*)&pixel.g,1);
                ss.write((char*)&pixel.b,1); 
          }
      }
        filename << "res-" << f << ".ppm";
        auto s = std::ofstream(filename.str());
        s << ss.str();
        s.close();
        filename.str("");
        ss.str("");
    }
}
