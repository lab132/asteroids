#version 150

// Uniforms
// ========
uniform vec2 u_origin;
uniform float u_rotation; // radians
uniform mat4 u_view;
uniform mat4 u_projection;

// Input
// =====
in vec2 vs_position;
in vec2 vs_texCoords;

// Output
// ======
out vec2 fs_texCoords;

// Functions
// =========
void main()
{
  vec4 transformedPos;
  transformedPos.x = u_origin.x + vs_position.x * cos(u_rotation) - vs_position.y * sin(u_rotation);
  transformedPos.y = u_origin.y + vs_position.x * sin(u_rotation) + vs_position.y * cos(u_rotation);
  transformedPos.z = 0.0;
  transformedPos.w = 1.0;

  fs_texCoords = vs_texCoords;
  gl_Position = u_projection
              * u_view
              * transformedPos;
}
