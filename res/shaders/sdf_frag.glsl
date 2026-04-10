#version 450 core

struct HitSurface
{
    float dist;
    vec3 color;
};

struct Plane 
{
    vec3 n;
    float d;
};

struct Sphere
{
    vec3 center;
    float radius;
    vec3 color;
};

struct Box
{
    vec3 position;
    vec3 b;
    float r;
    vec3 color;
};

uniform Sphere uSpheres[64];
uniform int uSphereCount;

uniform Box uBoxes[64];
uniform int uBoxCount;

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

float sdRoundBox(vec3 position, vec3 p, vec3 b, float r )
{
    
  vec3 q = abs(position-p) - b + r;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

HitSurface opUnion(HitSurface a, HitSurface b)
{
    return (a.dist < b.dist) ? a : b;
}

HitSurface opIntersection(HitSurface a, HitSurface b)
{
    return (a.dist > b.dist) ? a : b;
}

HitSurface opSmoothUnion(HitSurface a, HitSurface b, float k ) 
{
    HitSurface hit;
    float h = clamp( 0.5 + 0.5*(b.dist-a.dist)/k, 0.0, 1.0 );
    hit.dist = mix(b.dist, a.dist, h ) - k*h*(1.0-h);
    hit.color = mix(b.color, a.color, h );
    return hit;
}

HitSurface sdScene(vec3 position)
{
    HitSurface hit;
    hit.dist = 1e20;
    for (int i = 0; i < uSphereCount; ++i)
    {
        HitSurface sphereHit;
        sphereHit.dist = sdSphere(position, uSpheres[i].center, uSpheres[i].radius);
        sphereHit.color = uSpheres[i].color;

        hit = opSmoothUnion(hit, sphereHit,0.5);
    }

    for (int i = 0; i < uBoxCount; ++i)
    {
        HitSurface boxHit;
        boxHit.dist = sdRoundBox(position, uBoxes[i].position, uBoxes[i].b, uBoxes[i].r);
        boxHit.color = uBoxes[i].color;

        hit = opSmoothUnion(hit, boxHit, 0.5);
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
        float ambient = 0.2;

        vec3 color = (diffuse + ambient) * hit.color;
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(uBackgroundColor, 1.0);
    }
}