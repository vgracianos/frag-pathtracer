float csg(vec3 p, vec3 x) {
  p -= x;
  float d = sphere(p,vec4(0,0,0,1.35));
  d = max(d, box(p, vec3(0), vec3(1)));
  d = max(d, .7-length(p.xy));
  d = max(d, .7-length(p.xz));
  d = max(d, .7-length(p.yz));
  return d;
}
