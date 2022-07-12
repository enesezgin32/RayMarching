#version 410 core

out vec4 fragColor;

uniform float time;
uniform vec3 resolution;

uniform vec3 camPos;
uniform vec3 camDir;
uniform mat4 transposedCamView;

uniform vec3 ballPos[1001];
uniform float visibleSize;
uniform int ballAmount;
uniform float blending;

const float MAX_MARCHING_STEPS =  100.;
const float SURFACE_DIST= 0.001;
const float MAX_DIST= 100.;

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

float planeSDF( vec3 p, vec3 n, float h )
{
  return dot(p,n) + h;
}
float sphereSDF(vec3 p, vec3 position, float size)
{
    return length(p - position) - size;
}

float sceneSDF(vec3 p){

    float d = MAX_DIST+1;
    for(int i=0;i<ballAmount;i++)
    {
        d =  opSmoothUnion(d,sphereSDF(p, ballPos[i], visibleSize), blending);
    }
    return d;
}

float RayMarch(vec3 origin, vec3 direction)
{
    float dist = 0;
    for(int i=0; i<MAX_MARCHING_STEPS ;i++)
    {
        vec3 point = origin + direction * dist;
        float distanceToClosest = sceneSDF(point);
        dist += distanceToClosest;
        if(abs(distanceToClosest)<SURFACE_DIST || dist>MAX_DIST)
            break;
    }
    return dist;
}

vec3 GetNormal(vec3 p)
{
    float d = sceneSDF(p);
    vec2 e = vec2(0.01, 0);
    vec3 n = vec3(d) - vec3(
        sceneSDF(p-e.xyy),
        sceneSDF(p-e.yxy),
        sceneSDF(p-e.yyx)
    );
    return normalize(n);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy-0.5*resolution.xy) / resolution.y;
    vec3 ro = camPos;
    //Add little uv to camDir via converting cameraspace to worldspace
    vec3 rd = normalize(camDir + (transposedCamView*vec4(uv,0,0)).xyz);

    float d = RayMarch(ro,rd);
    vec3 p = ro + rd * d;
    if(d>MAX_DIST-SURFACE_DIST)
    {
        fragColor = vec4(vec3( 1 + rd.y), 0.0) * 0.3;
        return;
    }
    vec3 n = GetNormal(p);
    vec3 color = 0.5+ 0.5*n;
    fragColor = vec4(color, 1.0);
}