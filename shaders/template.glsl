#version 330
precision highp float;

layout(location = 0) out vec3 outColor;

uniform float time;
uniform vec2 iResolution;
uniform int nLights;
uniform uint sampleNumber;

#define EPS 0.01
#define EPS2 0.025
#define FAR 150
#define ITERATIONS 255
#define BOUNCES 15
#define MAX_ARRAY 16
#define INV_PI 0.31830988618
#define TWO_PI 6.28318530718
#define PI 3.14159265359

uniform sampler2D iChannel[MAX_ARRAY];
float textureScale[MAX_ARRAY];

struct Light {
  int type; // 0 = point, 1 = sphere
  vec3 p, col; // position and color emission
  float r; // r = radius
};
Light lights[MAX_ARRAY];
float lightCDF[MAX_ARRAY], lightPDF[MAX_ARRAY];

struct Properties {
  vec3 emission;
  float alpha, kr, kt, ior;
};
Properties properties[MAX_ARRAY];
vec3 solidColors[MAX_ARRAY];
vec3 checkerColorA[MAX_ARRAY];
vec3 checkerColorB[MAX_ARRAY];
float checkerSize[MAX_ARRAY];

float map(vec3 p);
void buildCamera(out vec3 ro, out vec3 rd);
void buildMaterialsAndLights();
ivec3 selectMaterial(vec3 p);

uint seed;

uint LCG(uint x) {
  return (1103515245u * x + 12345u) & 0x7fffffffu;
}

uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

float rand() {
  seed = LCG(seed);
  return clamp(float(seed & 0x3fffffffu) / float(0x3fffffffu), 0.0, 1.0);
}


// Calcula a normal com base no gradiente da função de distância.
vec3 calcNormal(vec3 p) {
  float f = sign(map(p));
  vec2 q = vec2(0.0, EPS);
  return normalize(f*vec3(map(p + q.yxx) - map(p - q.yxx),
                          map(p + q.xyx) - map(p - q.xyx),
                          map(p + q.xxy) - map(p - q.xxy)));
}

float shadowcast(vec3 ro, vec3 rd, float tmax) {
  float t = 0.0, d = 0.0;
  float fsign = sign(map(ro));
  for (float t = 0; t < tmax; ) {
    d = fsign * map(ro + t * rd);
    if (d < EPS)
      return 0;
    t += d;
  }
  return 1;
}

float shadowcastArea(vec3 ro, vec3 rd, float tmax, int id) {
  float t = 0.0, d = 0.0;
  float fsign = sign(map(ro));
  for (float t = 0; t < tmax; ) {
    d = fsign * map(ro + t * rd);
    if (d < EPS)
      break;
    t += d;
  }

  ivec3 mat = selectMaterial(ro + t*rd);
  return (mat.z == id) ? 1.0 : 0.0;
}

float raycast(vec3 ro, vec3 rd) {
  float pixelRadius = 1.0 / iResolution.y;
  
  float functionSign = sign(map(ro));
  float omega = 1.2, stepLength = 0, previousRadius = 0;

  float candidate_error = 10.0*FAR, candidate_t = 0.0, t = 0.0;

  for (int i = 0; i < ITERATIONS; ++i) {
    float signedRadius = functionSign * map(ro + t*rd);
    float radius = abs(signedRadius);

    bool sorFail = omega > 1 && (radius + previousRadius) < stepLength;

    if (sorFail) {
      stepLength -= omega * stepLength;
      omega = 1.0;
    } else {
      stepLength = signedRadius * omega;
    }
    previousRadius = radius;
    
    float error = radius / t;
    if (!sorFail && error < candidate_error) {
      candidate_t = t;
      candidate_error = error;
    }

    if (!sorFail && error < pixelRadius || t > FAR)
      break;
    t += 0.8*stepLength;
  }
  if (t > FAR || candidate_error > pixelRadius)
    return -1.0;
  return t;
}

vec3 checkerTexture(vec3 p, int id) {
  p /= checkerSize[id];
  float k = mod(floor(p.x) + floor(p.y) + floor(p.z), 2.0);
  return mix(checkerColorA[id], checkerColorB[id], k);
}

vec3 cubeMap(vec3 p, vec3 n, int id) {
  p /= textureScale[id];
  vec3 a = pow(texture2D(iChannel[id], p.yz).rgb,vec3(2.2));
  vec3 b = pow(texture2D(iChannel[id], p.xz).rgb,vec3(2.2));
  vec3 c = pow(texture2D(iChannel[id], p.xy).rgb,vec3(2.2));
  n = abs(n);
  return (a*n.x + b*n.y + c*n.z)/(n.x+n.y+n.z);   
}

void getProperties(in vec3 p, in vec3 n, in ivec3 mat, out vec3 tex, out Properties pr) {
  if (mat.x == 0) // solid
    tex = solidColors[mat.y];
  else if (mat.x == 1) // checkerboard
    tex = checkerTexture(p, mat.y);
  else if (mat.x == 2) // texmap
    tex = cubeMap(p, n, mat.y);
  pr = properties[mat.z];
}

float fresnel(float ior, float cosTheta) {
  // Aproximação de Schlick
  // assumindo que o meio de transmissão é o ar!
  float r0 = (1.0 - ior) / (1.0 + ior);
  r0 *= r0;
  return r0 + (1.0 - r0) * pow(1.0 - max(0, cosTheta), 5.0);
}

vec3 fresnel(vec3 r0, float cosTheta) { // versão para blinn-phong
  // Aproximação de Schlick
  // assumindo que o meio de transmissão é o ar!
  return r0 + (1.0 - r0) * pow(1.0 - max(0, cosTheta), 5.0);
}

vec3 optimizeHit(vec3 p, vec3 rd) {
  for (int i = 0; i < 10; ++i)
    p += rd * (abs(map(p)) - 0.01/iResolution.y*length(p));
  return p;
}

vec3 toWorldSpace(vec3 n, vec3 w) {
  vec3 t = abs(n.x) > abs(n.y) ? vec3(n.z, 0.0, -n.x) : vec3(0, -n.z, n.y);
  t = normalize(t);
  vec3 b = normalize(cross(n, t));
  return normalize(mat3(b, n, t) * w);
}

vec3 cosineWeightedSample() {
  // Malley's method.
  vec3 w;
  float r = sqrt(max(0, rand()));
  float theta = TWO_PI*rand();
  w.x = r * cos(theta);
  w.z = r * sin(theta);
  w.y = sqrt(max(0.0, 1.0 - w.x*w.x - w.z*w.z));
  return w;
}

float blinnBRDF(float alpha, float cosH) {
  float k = (alpha + 2.0)*(alpha + 4.0);
  k /= 8 * PI * (pow(2.0, -0.5*alpha) + alpha);
  return k * pow(cosH, alpha);
}

float blinnPDF(float alpha, float cosH, float cosWoH) {
  return clamp((alpha + 1)/TWO_PI * pow(cosH, alpha) / (4.0 * cosWoH), 0, 1);
}

vec3 blinnSample(float alpha) {
  vec3 h;
  float phi = TWO_PI*rand();
  h.y = pow(rand(), 1.0 / (alpha + 1.0));
  h.x = h.z = sqrt(max(0, 1.0 - h.y*h.y));
  h.x *= cos(phi); h.z *= sin(phi);
  return h;
}

void buildLightDistribution() {
  float sum = 0;
  for (int i = 0; i < MAX_ARRAY; ++i) {
    if (i >= nLights) break;
    vec3 c = lights[i].col;
    float emit = max(c.r, max(c.g, c.b));
    switch(lights[i].type) {
    case 0: // point light.
      lightPDF[i] = 4.0 * PI * emit;break;
    case 1: // sphere light.
      lightPDF[i] = PI * emit * 4 * PI * pow(lights[i].r, 2.0); break;
    }
    sum += lightPDF[i];
  }

  for (int i = 0; i < MAX_ARRAY; ++i) {
    if (i >= nLights) break;
    lightPDF[i] /= sum;
  }
}

int sampleLightIndex() {
  float sum = 0;
  float k = rand();
  for (int i = 0; i < MAX_ARRAY; ++i) {
    if (i >= nLights) return i - 1;
    sum += lightPDF[i];
    if (k < sum) return i;
  }
  return 0;
}

/*vec3 sampleDiskLight(vec3 p, vec3 n, float r) {
  vec3 w = vec3(0);
  float theta = TWO_PI*rand();
  r = sqrt(r*rand());
  w.x = r * cos(theta);
  w.z = r * sin(theta);
  return toWorldSpace(n, w) + p; 
  }*/

/*vec3 sampleSphere(vec3 p, float radius) {
  float z = 1 - 2*rand();
  float r = sqrt(max(0, 1 - z*z));
  float phi = TWO_PI * rand();
  return radius*vec3(r*cos(phi), r*sin(phi), z) + p;
  }*/

vec3 sampleCone(float cosThetaMax) {
  float u1 = rand();
  float cosTheta = (1 - u1) + u1 * cosThetaMax;
  float sinTheta = sqrt(max(0, 1 - cosTheta*cosTheta));
  float phi = TWO_PI * rand();
  return vec3(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
}

float iSphere( in vec3 ro, in vec3 rd, in vec4 sph ) {
  vec3 oc = ro - sph.xyz;
  float b = dot(oc, rd);
  float c = dot(oc, oc) - sph.w * sph.w;
  float h = b * b - c;
  if (h < 0.0) return -1.0;
  
  float s = sqrt(h);
  float t1 = -b - s;
  float t2 = -b + s;
  
  return t1 < 0.0 ? t2 : t1;
}

vec3 sampleSphere(vec3 p, vec3 c, float radius, inout float pdf) {
  vec3 l = c - p;
  float sinThetaMax2 = radius * radius / dot(l, l);
  float cosThetaMax = sqrt(max(0, 1 - sinThetaMax2));
  vec3 s = sampleCone(cosThetaMax);
  s = toWorldSpace(normalize(l), s);
  float t = iSphere(p, s, vec4(c, radius));
  if (t == -1)
    t = max(0, dot(c - p, s));
  pdf = (abs(cosThetaMax - 1) < 1E-8) ? 0 : 1.0 / (TWO_PI * (1.0 - cosThetaMax));
  pdf = clamp(pdf, 0, 1);
  return p + t * s;
}

vec3 directLight(vec3 ro, vec3 rd, float t, vec3 n, vec3 p, vec3 tex, Properties pr) {
  if (map(p) < 0 || pr.kr > 0 || pr.kt > 0)
    return vec3(0.0);

  float pdf = 0;
  vec3 col = vec3(0.0), lightPos = vec3(0);
  
  int i = sampleLightIndex();

  switch(lights[i].type) {
  case 0: lightPos = lights[i].p; pdf = 1; break;
  case 1: lightPos = sampleSphere(p, lights[i].p, lights[i].r + 2.0*EPS2, pdf);
    if (pdf == 0) return vec3(0); break;
  }
  vec3 l = lightPos - p;
  float d = sqrt(dot(l, l)); l /= d;
  float lamb = dot(n, l);

  if (lamb <= 0) return col;
  lamb = abs(lamb);

  if (lights[i].type == 1) {
    vec3 ln = calcNormal(lightPos);
    if (dot(ln, -l) <= 0) return vec3(0.0);
  }
  
  float shadow = shadowcast(p + max(EPS2, 2.0*abs(map(p)))*n, l, d);
  vec3 h = normalize(l - rd);
  vec3 fre = fresnel(tex, dot(h, -rd));
  float prob = max(fre.r, max(fre.g, fre.b));
  if (pr.alpha > 0 && rand() < prob) {
    // blinn-phong
    col += fre * blinnBRDF(pr.alpha, abs(dot(n, h))) / prob;
  } else if (pr.alpha > 0) {
    // difuso blinn-phong
    col += (1.0-fre) * tex * INV_PI / (1.0-prob);
  } else {
    // difuso
    col += tex*INV_PI;
  }
  col *= shadow * lamb * lights[i].col;
  col /= d * d * lightPDF[i] * pdf;
  return col + pr.emission;
}

vec3 getBgColor(vec3 rd) {
  return 0.5*vec3(0.7, 0.8, 1.0)*(1.0-0.5*rd.y);
}

vec3 raytrace(vec3 ro, vec3 rd) {
  vec3 L = vec3(0);
  vec3 pathThroughput = vec3(1);

  //float pathDistance = 0.0;
  bool specularBounce = false;
  
  for (int i = 0; i < BOUNCES; ++i) {
    float t = raycast(ro, rd);
    if (t < 0) {
      // calcula iluminação direta para luzes especulares
      // somente se o último raio a bater for especular.
      if (specularBounce)
        for (int i = 0; i < MAX_ARRAY; ++i) {
          if (i >= nLights) break;
          if (lights[i].type == 0) {
            float k = dot(lights[i].p - ro, lights[i].p - ro);
            L += pathThroughput * lights[i].col / k;
          }
        }
      L += pathThroughput * getBgColor(rd);
      break;
    }
   
    // Informações do ponto de colisão.
    vec3 p = optimizeHit(ro + t*rd, rd);
    vec3 n = calcNormal(p);
    //pathDistance += length(p - ro);
      
    // Informações do material.
    vec3 tex; Properties pr;
    ivec3 mat = selectMaterial(p);
    getProperties(p, n, mat, tex, pr);

    if (i == 0 || specularBounce)
      L+= pathThroughput * pr.emission;
    L += pathThroughput * directLight(ro, rd, t, n, p, tex, pr);
   
    float flip = 1;
    if (pr.kt > 0) {
      // Transmissão (BSTF) com reflexões internas e fresnel
      float ior = (map(p) < 0) ? 1.0/pr.ior : pr.ior;
      float fre = fresnel(ior, dot(-rd, n));

      if (rand() < fre) {
        pathThroughput *= tex;
        rd = reflect(rd, n);
      } else {
        rd = refract(rd, n, 1.0/ior);
        if (rd == vec3(0.0))
          break;
        pathThroughput *= tex*ior*ior;
        flip = -1;
      }
      specularBounce = true;
    } else if (pr.kr > 0) {
      // Especular (BRDF) com fresnel
      pathThroughput *= fresnel(tex, dot(-rd, n));
      rd = reflect(rd, n);
      specularBounce = true;
    } else if (pr.alpha > 0) {
      // Blinn-Phong BRDF com fresnel
      vec3 h = blinnSample(pr.alpha);
      float cosH = abs(h.y);
      vec3 fre = fresnel(tex, cosH);
      float prob = max(fre.r, max(fre.g, fre.b));
      if (rand() < prob) {
        h = toWorldSpace(n, h);
        float cosWoH = abs(dot(-rd, h));
        rd = reflect(rd, h);
        float pdf = blinnPDF(pr.alpha, cosH, cosWoH);
        if (pdf <= 1E-6) break;
        pathThroughput *= fre * blinnBRDF(pr.alpha, cosH) * max(0, dot(n, rd));
        pathThroughput /= pdf * prob;
      } else {
        rd = cosineWeightedSample();
        rd = toWorldSpace(n, rd);
        pathThroughput *= tex * (1.0-fre) / (1.0 - prob);
      }
      specularBounce = false;
    } else {
      // Difuso (BRDF)
      rd = cosineWeightedSample();
      rd = toWorldSpace(n, rd);
      pathThroughput *= tex;
      specularBounce = false;
    }

    // roleta russa
    if (i > 5) {
      float k = max(pathThroughput.r, max(pathThroughput.g, pathThroughput.b));
      float continueProbability = min(.8, k);
      if (rand() > continueProbability)
        break;
      pathThroughput /= continueProbability;
    }

    pathThroughput = clamp(pathThroughput, 0.0, 1.0);
    ro = p + flip*max(EPS2, 2.0*abs(map(p)))*n;
  }
  
  return L;
}

void main() {
  vec3 ro, rd;
  vec2 uv = gl_FragCoord.xy/iResolution.xy;

  float lastRand = texture2D(iChannel[1], uv).r;
  
  seed = uint(gl_FragCoord.y * iResolution.y + gl_FragCoord.x);
  seed = wangHash(seed + wangHash(sampleNumber));
  
  buildCamera(ro, rd);
  buildMaterialsAndLights();
  buildLightDistribution();
  
  vec3 col = raytrace(ro, rd);
  
  // Moving average.
  col += sampleNumber * texture2D(iChannel[0], uv).rgb;
  col /= sampleNumber + 1u;

  outColor = col;
}

float sphere(vec3 p, vec4 sph) {
  return length(p - sph.xyz) - sph.w;
}

float plane(vec3 p, vec4 pln) {
  return dot(vec4(p,1), pln)/length(pln.xyz);
}

float box(vec3 p, vec3 x, vec3 b) {
  p -= x;
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0));
}

float torus(vec3 p, vec3 x, vec2 t) {
  p -= x;
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float cone(vec3 p, vec3 x, vec2 c) {
  p -= x;
  float q = length(p.xz);
  return dot(normalize(c),vec2(q,p.y));
}

float cylinder(vec3 p, vec3 x, vec2 h) {
  p -= x;
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float disk(vec3 p, vec3 x, vec3 n, float r) {
  p -= x;
  float l = length(p - dot(p, n)*n);
  return max(l - r,  abs(plane(p, vec4(n, 0))));
}
