#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float iTime;
uniform vec2  iResolution;
uniform vec4  iMouse;
uniform sampler2D texture0;

out vec4 finalColor;

// Original - https://www.shadertoy.com/view/ldBfDd
void main()
{
    vec2 fragCoord = fragTexCoord * iResolution;

    // Get pos relative to 0-1 screen space
    vec2 uv = fragCoord.xy / iResolution.xy;

    // Map texture to 0-1 space
    vec4 texColor = texture(texture0,uv);

    // Default lcd colour (affects brightness)
//     float pb = 0.4;
    float pb = 0.85;
    vec4 lcdColor = vec4(pb,pb,pb,1.0);

    // Change every 1st, 2nd, and 3rd vertical strip to RGB respectively
    int px = int(mod(fragCoord.x,3.0));
	if (px == 1) lcdColor.r = 1.0;
    else if (px == 2) lcdColor.g = 1.0;
    else lcdColor.b = 1.0;

    // Darken every 3rd horizontal strip for scanline
//     float sclV = 0.25;
    float sclV = 0.85;
    if (int(mod(fragCoord.y,3.0)) == 0) lcdColor.rgb = vec3(sclV,sclV,sclV);

    finalColor = texColor*lcdColor;
}