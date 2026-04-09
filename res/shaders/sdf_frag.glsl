#version 450 core

struct Sphere
{
    vec3 center;
    float radius;
};

uniform Sphere uSpheres[64];
uniform int uSphereCount;


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

float sdScene(vec3 position)
{
    float minDist = 1e20;
    for (int i = 0; i < uSphereCount; ++i)
    {
        float d = sdSphere(position, uSpheres[i].center, uSpheres[i].radius);
        minDist = min(minDist, d);
    }

    return minDist;
}

float raymarch(vec3 ro, vec3 rd)
{
    float t = 0.0;

    for (int i = 0; i < MAX_STEPS; ++i)
    {
        vec3 p = ro + rd * t;
        float d = sdScene(p);
    
        if (d < EPSILON)
            return t;
    
        t += d;
    
        if (t > MAX_DIST)
            break;
    }
    return 0.0;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / uResolution) * 2.0 - 1.0;

    vec4 clip = vec4(uv, 1.0, 1.0);
    vec4 world = uInverseViewProj * clip;
    world /= world.w;

    vec3 ro = uCameraPos;
    vec3 rd = normalize(world.xyz - ro);


	float t = raymarch(ro, rd);

    if (t > 0.0)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}