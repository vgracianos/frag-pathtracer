float smin(float a, float b, float k) {
  float h = clamp(.5+.5*(b-a)/k, 0.0, 1.0 );
  return mix(b,a,h)-k*h*(1.-h);
}

float line(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length(pa - ba*h) - r;
}

float lattice(vec3 p, vec3 x) {
  p -= x;
  float c = cos(0.35*PI), s = sin(0.35*PI);
  mat2 rot = mat2(c,-s,s,c);
  p.xz = rot * p.xz;
  p.xy = rot * p.xy;

  const float r = .5;
  const float l = 0.5 * 3 / 1.1547;
  const float k = l * 0.866;

  vec4 p1 = vec4(0,k,0,r);
  vec4 p2 = vec4(0,-k,l,r);
  vec4 p3 = vec4(l,-k,-l,r);
  vec4 p4 = vec4(-l,-k,-l,r);
    
  float d = sphere(p,p1);
  d = min(d, sphere(p,p2));
  d = min(d, sphere(p,p3));
  d = min(d, sphere(p,p4));

  d = smin(d, line(p, p1.xyz, p2.xyz, 0.35*r), 0.35);
  d = smin(d, line(p, p1.xyz, p3.xyz, 0.35*r), 0.35);
  d = smin(d, line(p, p1.xyz, p4.xyz, 0.35*r), 0.35);
  d = smin(d, line(p, p2.xyz, p3.xyz, 0.35*r), 0.35);
  d = smin(d, line(p, p2.xyz, p4.xyz, 0.35*r), 0.35);
  d = smin(d, line(p, p3.xyz, p4.xyz, 0.35*r), 0.35);
  return d;
}
