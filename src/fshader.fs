#version 330 core
out vec4 FragColour;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform float glow;
float edge0 = 0.0;
float edge1 = 2.0;

void main()
{
   vec4 colour = texture(ourTexture, TexCoord);
   if (colour.a < 1)
   {
      discard;
   }

   float intensity = smoothstep(edge0, edge1, glow);
   
   FragColour = colour + vec4(intensity);
}