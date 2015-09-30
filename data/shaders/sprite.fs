#version 150

// Uniforms
// ========
uniform sampler2D u_texture;
uniform vec4 u_color;

// Input
// =====
in vec2 fs_texCoords;

// Output
// ======
out vec4 out_color;

// Functions
// =========
void main()
{
  vec4 texColor = texture(u_texture, fs_texCoords);
  if(texColor.a < 0.1)
  {
    discard;
  }
  out_color = texColor * u_color;
}
