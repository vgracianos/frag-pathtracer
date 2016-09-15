#version 330
precision highp float;

out vec4 color;

uniform vec2 iResolution;
uniform sampler2D blitTexture;

// Referencia http://filmicgames.com/archives/75

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x) {
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() {
  vec2 uv = gl_FragCoord.xy / iResolution.xy;
  vec3 tex = 4*texture2D(blitTexture, uv).rgb;

  vec3 curr = Uncharted2Tonemap(tex);

  vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
  vec3 col = curr * whiteScale;
  
  color = vec4(pow(col, vec3(1.0/2.2)), 1);
}
