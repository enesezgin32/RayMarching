#version 410 core

out vec4 fragColor;

uniform float time;
uniform vec3 resolution;

uniform vec3 camPos;
uniform vec3 camDir;
uniform mat4 transposedCamView;

const float PI = 3.14159265359;

// weight of the components
// in this case, we can pass separate values from the main application even if Ka+Kd+Ks>1. In more "realistic" situations, I have to set this sum = 1, or at least Kd+Ks = 1, by passing Kd as uniform, and then setting Ks = 1.0-Kd
uniform float Ka;
uniform float Kd;
uniform float Ks;

// shininess coefficients (passed from the application)
uniform float shininess;

const float MAX_MARCHING_STEPS =  100.;
const float SURFACE_DIST= 0.001;
const float MAX_DIST= 200.;

float curItem;

float sphereSDF(vec3 p, vec3 position, float size)
{
    return length(p - position) - size;
}
float sdBox( vec3 p, vec3 b ,vec3 position)
{
    p-= position;
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
float sdTorus( vec3 p, vec2 t , vec3 pos)
{
    p-=pos;
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float sceneSDF(vec3 p){
    float d=MAX_DIST;
    float f;
    d = sphereSDF(p, vec3(0), 1);
    d = min(d,sdBox(p, vec3(1), vec3(2.2,0,0)));
    d = min(d,sdTorus(p, vec2(1, 0.3), vec3(1.2,0,-3)));
    f = (p + vec3(0,1,0)).y + 0.1;
    if(f<d)
    {
        curItem = 1;
        return f;
    }
    else
    {
        curItem = 2;
        return d;
    }
}

float RayMarch(vec3 origin, vec3 direction)
{
    float dist = 0.5;
    float i;
    for(i=0; i<MAX_MARCHING_STEPS ;i++)
    {
        vec3 point = origin + direction * dist;
        float distanceToClosest = sceneSDF(point);
        dist += distanceToClosest;
        if(abs(distanceToClosest)<SURFACE_DIST || dist>MAX_DIST)
            break;
    }
    if(dist > MAX_DIST)
        return -1;
    return dist;
}

vec3 GetNormal(vec3 p)
{
    float d = sceneSDF(p);
    vec2 e = vec2(0.0001, 0);
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
    if(d<0)
    {
        vec3 skyColor = vec3(0.5, 0.8, 0.9) - max(rd.y,0.0)*0.5;
        fragColor = vec4(skyColor, 1.0);
        return;
    }

    vec3 color = vec3(0.0);
    vec3 n = GetNormal(p);
    vec3 sunPos = vec3(0.8, 0.4, -0.2);

    
    vec3 material = vec3(0.15); // base color for objects, albino
    if(curItem == 1)
        material = vec3(0.01);
    vec3 sunDir = normalize(sunPos);//sun comes parallel 
    float sunDiffuse = clamp(dot(n, sunDir), 0, 1);//More directed to sun, more effected
    float sunShadow = step(RayMarch(p + n*0.001, sunPos),0.0); //if sees light 1, else 0 so eliminates sun addition
    float skyDiffuse = clamp(0.5 + 0.5*dot(n, vec3(0,1,0)), 0, 1);//light comes from directly up, more directed to up, more effected
    float bounceDiffuse = clamp(0.5 + 0.5*dot(n, vec3(0,-1,0)), 0, 1);//light comes from directly down, more directed to down, more effected, to eliminite full blackness of close to ground objects
    color += material * vec3( 7, 5, 3) * sunDiffuse * sunShadow;
    color += material * vec3( 0.5, 0.8, 0.9) * skyDiffuse;
    color += material * vec3( 0.7, 0.3, 0.2) * bounceDiffuse;

    color = pow(color, vec3(0.4545)); // Gamma correction
    fragColor = vec4(color, 1.0);
}