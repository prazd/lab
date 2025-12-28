#ifndef MAIN_CPP_GLSL
#define MAIN_CPP_GLSL
namespace glsl {
    struct mat3{float m[3][3];};

    struct vec3 {
     float x,y,z;
     vec3 operator*(float v);
     vec3 operator*(vec3 v);
     vec3 operator+(float v);
     vec3 operator-(float v);
     vec3& operator*=(vec3 v);
     vec3 operator-(vec3 v);
     bool operator<(vec3 v)const;
     vec3 operator+(vec3 v);
     float length() const;
    };

    struct vec2{
        float x,y;
        vec2 operator-(vec2 v);
        vec2 operator-(float v);
        vec2 operator*(float v);
        vec2 operator+(float v);
        vec2 operator/(float v);
        float length() const;
    };
    vec3 mode_vec3(vec3 vec, float n);
    float smoothstep(float edge0, float edge1, float x);
    float get_i_time();

    vec3 mul_mat3(const mat3& m, vec3 v);

    struct pixel{
        unsigned char r{};
        unsigned char g{};
        unsigned char b{};
    };

    vec3 max_vec3(vec3 a, vec3 b);
    float mod(float x, float y);

    float dot_vec3(vec3,vec3);

    unsigned char color_to_byte(float c);
}
#endif