#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out float smoothness;
uniform vec4 midpoint;

uniform mat4 model;
uniform int glow;
void main()
{
   gl_Position = model * vec4(aPos, 1.0);
   TexCoord = aTexCoord;
   smoothness = 0;
   float dist = 0;
   dist = sqrt((gl_Position.x - midpoint[0])*(gl_Position.x - midpoint[0]) + (gl_Position.y - midpoint[1])*(gl_Position.y - midpoint[1]));
   if (glow != 0)
      smoothness = smoothstep(0.0, 1.0, dist)/2;
}