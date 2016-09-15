#define TAU 6.2831

float knot(vec3 p, vec3 x) {
  p -= x;
  float r = length(p.xy);
  float oa, a = atan(p.y, p.x); oa = 1.5*a;
  a = mod(a, 0.001*TAU) - 0.001*TAU/2.0;
  p.xy = r*vec2(cos(a), sin(a)); p.x -= 6.0;
  p.xz = cos(oa)*p.xz + sin(oa)*vec2(-p.z, p.x);
  p.x = abs(p.x) - 1.35; 
  return 0.5*(length(p) - 1.0);
}
