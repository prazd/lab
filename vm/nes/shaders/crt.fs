#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float iTime;
uniform vec2  iResolution;
uniform vec4  iMouse;
uniform sampler2D texture0;

out vec4 finalColor;

// Original - https://www.shadertoy.com/view/XtlSD7
#define RGB( r, g, b ) vec3( float( r ) / 255.0, float( g ) / 255.0, float( b ) / 255.0 )
vec2 CRTCurveUV( vec2 uv )
{
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs( uv.yx ) / vec2( 6.0, 4.0 );
    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void DrawVignette( inout vec3 color, vec2 uv )
{
    float vignette = uv.x * uv.y * ( 1.0 - uv.x ) * ( 1.0 - uv.y );
    vignette = clamp( pow( 16.0 * vignette, 0.3 ), 0.0, 1.0 );
    color *= vignette;
}

void DrawScanline( inout vec3 color, vec2 uv )
{
    float scanline 	= clamp( 0.95 + 0.05 * cos( 3.14 * ( uv.y + 0.008 * iTime ) * 240.0 * 1.0 ), 0.0, 1.0 );
    float grille 	= 0.85 + 0.15 * clamp( 1.5 * cos( 3.14 * uv.x * 640.0 * 1.0 ), 0.0, 1.0 );
    color *= scanline * grille * 1.2;
}

void main(){
    vec2 fragCoord = fragTexCoord * iResolution;

    vec3 color = texture(texture0, fragTexCoord).rgb;
     // we want to see at least 224x192 (overscan) and we want multiples of pixel size
    float resMultX  = floor( iResolution.x / 224.0 );
    float resMultY  = floor( iResolution.y / 192.0 );
    float resRcp	= 1.0 / max( min( resMultX, resMultY ), 1.0 );

    float time			= iTime;
    float screenWidth	= floor( iResolution.x * resRcp );
    float screenHeight	= floor( iResolution.y * resRcp );
    float pixelX 		= floor( fragCoord.x * resRcp );
    float pixelY 		= floor( fragCoord.y * resRcp );

    // CRT effects (curvature, vignette, scanlines and CRT grille)
    vec2 uv    = fragCoord.xy / iResolution.xy;
    vec2 crtUV = CRTCurveUV( uv );
    if ( crtUV.x < 0.0 || crtUV.x > 1.0 || crtUV.y < 0.0 || crtUV.y > 1.0 )
    {
        color = vec3( 0.0, 0.0, 0.0);
    }
    DrawVignette( color, crtUV );
    DrawScanline( color, uv );

    finalColor = vec4(color.r, color.g, color.b, 1.0);
}