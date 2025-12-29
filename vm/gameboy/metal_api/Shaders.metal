#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut vertex_main(uint vertexID [[vertex_id]]) {
    float4 positions[3] = { {-1,-1,0,1}, {3,-1,0,1}, {-1,3,0,1} };
    float2 texCoords[3] = { {0,0}, {2,0}, {0,2} };
    VertexOut out;
    out.position = positions[vertexID];
    out.texCoord = texCoords[vertexID];
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
texture2d<float> tex [[texture(0)]])
{
return tex.read(uint2(in.texCoord.x * tex.get_width(),
                      in.texCoord.y * tex.get_height()));
}
