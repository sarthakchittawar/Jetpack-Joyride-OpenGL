#version 330 core
out vec4 FragColour;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform int glow;

void main()
{
   vec4 colour = texture(ourTexture, TexCoord);
   float zapdist = 0, playerdist = 0;
   zapdist = abs(TexCoord.x - 0.5);
   playerdist = distance(TexCoord, vec2(0.5));
   if (colour.a < 1)
   {
      if (glow == 1)
         colour = mix(colour, vec4(1.0, 1.0, 1.0, 1.0), 1 - smoothstep(0.0, 0.6, playerdist));
      else if (glow == 2)
         colour = mix(colour, vec4(1.0, 1.0, 0.0, 1.0), 1 - smoothstep(0.0, 0.5, zapdist));
      else discard;
   }
   
   FragColour = colour;
}