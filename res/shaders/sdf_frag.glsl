#version 450 core

struct HitSurface
{
    float dist;
    vec3 color;
};

struct Sphere
{
    vec3 center;
    float radius;
    vec3 color;
};

uniform Sphere uSpheres[64];
uniform int uSphereCount;

uniform vec3 uLightPosition;

uniform vec3 uBackgroundColor;

uniform vec2 uResolution;

uniform mat4 uInverseViewProj;
uniform vec3 uCameraPos;

out vec4 FragColor;

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

float sdSphere(vec3 position, vec3 center, float radius)
{
    return length(position - center) - radius;
}

HitSurface sdScene(vec3 position)
{
    HitSurface hit;
    hit.dist = 1e20;
    for (int i = 0; i < uSphereCount; ++i)
    {
        float d = sdSphere(position, uSpheres[i].center, uSpheres[i].radius);
        if(d<hit.dist)
        {
            hit.dist = d;
            hit.color = uSpheres[i].color;
        }
    }
    return hit;
}

HitSurface raymarch(vec3 ro, vec3 rd)
{
    float t = 0.0;
    HitSurface hit;

    for (int i = 0; i < MAX_STEPS; ++i)
    {
        vec3 p = ro + rd * t;
        hit = sdScene(p);
    
        if (hit.dist < EPSILON)
        {
            hit.dist = t;
            return hit;
        }

        t += hit.dist;
    
        if (t > MAX_DIST)
        {
            break;
        }
    }

    hit.dist = 0.0;
    return hit;
}

vec3 getNormal(vec3 p)
{
    float e = 0.001;
    vec2 h = vec2(e, 0.0);

    return normalize(vec3(
        sdScene(p + vec3(h.x, h.y, h.y)).dist - sdScene(p - vec3(h.x, h.y, h.y)).dist,
        sdScene(p + vec3(h.y, h.x, h.y)).dist - sdScene(p - vec3(h.y, h.x, h.y)).dist,
        sdScene(p + vec3(h.y, h.y, h.x)).dist - sdScene(p - vec3(h.y, h.y, h.x)).dist
    ));
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / uResolution) * 2.0 - 1.0;

    vec4 clip = vec4(uv, 1.0, 1.0);
    vec4 world = uInverseViewProj * clip;
    world /= world.w;

    vec3 ro = uCameraPos;
    vec3 rd = normalize(world.xyz - ro);


	HitSurface hit = raymarch(ro, rd);

    if (hit.dist > 0.0)
    {
        vec3 p = ro + rd * hit.dist;
        vec3 n = getNormal(p);
        vec3 lightDir = normalize(uLightPosition - p);
        float diffuse = clamp(dot(n, lightDir), 0.0, 1.0);
        float ambient = 0.3;

        vec3 color = (diffuse + ambient) * hit.color;
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(uBackgroundColor, 1.0);
    }
}