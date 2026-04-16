#version 450 core

const int SHAPE_SPHERE = 0;
const int SHAPE_BOX    = 1;
const int SHAPE_PLANE  = 2;

const int OP_UNION        = 0;
const int OP_SMOOTH_UNION = 1;
const int OP_INTERSECTION = 2;
const int OP_SUBTRACTION  = 3;

struct HitSurface
{
    float dist;
    vec3 color;
    int tex;
};

struct Plane 
{
    vec3 n;
    float d;
    vec3 color;
    int tex;
};

struct Sphere
{
    vec3 center;
    float radius;
    vec3 color;
    int tex;
};

struct Box
{
    vec3 position;
    vec3 b;
    float r;
    vec3 color;
    int tex;
};

struct SceneOp
{
    int shapeType;
    int shapeIndex;
    int opType;
};

uniform SceneOp uSceneOps[16];
uniform int uSceneOpCount;

uniform Sphere uSpheres[8];

uniform Box uBoxes[8];

uniform Plane uPlanes[8];

uniform vec3 uLightPosition;

uniform vec3 uBackgroundColor;
uniform samplerCube uSkybox;

uniform vec2 uResolution;

uniform mat4 uInverseViewProj;
uniform vec3 uCameraPos;
uniform int uAA;

out vec4 FragColor;

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float EPSILON = 0.01;

// Distance Functions

float sdPlane(vec3 p, vec3 n, float h)
{
    return dot(p,n) + h;
}

float sdSphere(vec3 position, vec3 center, float radius)
{
    return length(position - center) - radius;
}

float sdRoundBox(vec3 position, vec3 p, vec3 b, float r )
{
    
  vec3 q = abs(position-p) - b + r;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

// Evaluate shape helper function
HitSurface evalShape(vec3 position, int shapeType, int shapeIndex)
{
    HitSurface hit;

    if (shapeType == SHAPE_SPHERE)
    {
        hit.dist = sdSphere(position, uSpheres[shapeIndex].center, uSpheres[shapeIndex].radius);
        hit.color = uSpheres[shapeIndex].color;
        hit.tex = uSpheres[shapeIndex].tex;
        return hit;
    }
    else if (shapeType == SHAPE_BOX)
    {
        hit.dist = sdRoundBox(position, uBoxes[shapeIndex].position, uBoxes[shapeIndex].b, uBoxes[shapeIndex].r);
        hit.color = uBoxes[shapeIndex].color;
        hit.tex = uBoxes[shapeIndex].tex;
        return hit;
    }
    else
    {
        hit.dist = sdPlane(position, uPlanes[shapeIndex].n, uPlanes[shapeIndex].d);
        hit.color = uPlanes[shapeIndex].color;
        hit.tex = uPlanes[shapeIndex].tex;
        return hit;
    }
}

// Operations

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
    hit.tex = (h>0.5) ? a.tex : b.tex;
    return hit;
}

HitSurface opSubtraction(HitSurface a, HitSurface b)
{
    HitSurface hit;
    hit.dist = max(a.dist, -b.dist);
    hit.color = a.color;
    hit.tex = a.tex;
    return hit;
}

HitSurface applyOp(HitSurface a, HitSurface b, int opType)
{
    if (opType == OP_UNION)        return opUnion(a, b);
    if (opType == OP_SMOOTH_UNION) return opSmoothUnion(a, b, 0.5); // todo: configure the smooth factor (stop hardcoding 0.5)
    if (opType == OP_INTERSECTION) return opIntersection(a, b);
    if (opType == OP_SUBTRACTION)  return opSubtraction(a, b);
    return opUnion(a, b);
}

HitSurface sdScene(vec3 position)
{
    HitSurface hit;
    hit.dist = 1e20;
    hit.color = vec3(0.0);
    hit.tex = 0;

    if (uSceneOpCount == 0)
        return hit;

    SceneOp first = uSceneOps[0];
    hit = evalShape(position, first.shapeType, first.shapeIndex);

    for (int i = 1; i < uSceneOpCount; ++i)
    {
        SceneOp entry = uSceneOps[i];
        HitSurface next = evalShape(position, entry.shapeType, entry.shapeIndex);
        hit = applyOp(hit, next, entry.opType);
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

// Simpler raymarch function for shadow
// from: https://iquilezles.org/articles/rmshadows/
float shadowRay(vec3 ro, vec3 rd, float maxDist, float k)
{
    float t = 0.1;
    //float pd = 1e20;
    float result = 1.0;

    for (int i = 0; i < 64; ++i)
    {
        vec3 p = ro + rd * t;
        float d = sdScene(p).dist;

        if (d < 0.001)
            return 0.0;

//        float y = d*d/(2.0*pd);
//        float dd = sqrt(d*d-y*y);
//        result = min(result, dd/(k*max(0.0,t-y)));
//        pd = d;

        result = min( result, k*d/t );
        t += d;
         
        if (t >= maxDist)
            break;
    }

    return clamp(result, 0.0, 1.0);
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

float checkerTexture(vec2 uv)
{
    vec2 c = floor(uv);
    return mod(c.x + c.y, 2.0);
}

vec3 renderSample(vec2 fragCoord)
{
    vec2 uv = (fragCoord / uResolution) * 2.0 - 1.0;

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

        vec3 toLight = uLightPosition - p;
        float lightDist = length(toLight);
        vec3 lightDir = normalize(toLight);

        float shadow = shadowRay(p + n * 0.01, lightDir, lightDist, 64.0);
        float diffuse = max(dot(n, lightDir), 0.0) * shadow;
        float ambient = 0.2;

        vec3 albedo = hit.color;

        float pattern = checkerTexture(p.xz * 0.5);
        vec3 checkerAlbedo = mix(albedo, albedo * 0.75, pattern);
        albedo = mix(albedo, checkerAlbedo, hit.tex);

        return albedo * (diffuse + ambient);
    }
    else
    {
        return texture(uSkybox, rd).rgb;
    }
}

void main()
{
    vec3 color = vec3(0.0);
    if(uAA == 1)
    {
        // 2Å~2 SSAA
        vec2 offsets[4] = vec2[](
            vec2(-0.25, -0.25),
            vec2( 0.25, -0.25),
            vec2(-0.25,  0.25),
            vec2( 0.25,  0.25)
        );

        for (int i = 0; i < 4; ++i)
        {
            color += renderSample(gl_FragCoord.xy + offsets[i]);
        }
        color *= 0.25;
    } 
    else 
    {
        color = renderSample(gl_FragCoord.xy);
    }
    FragColor = vec4(color, 1.0);
}