#version 330 core
out vec4 FragColour;

in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
   vec4 colour = texture(ourTexture, TexCoord);
   if (colour.a < 1)
   {
      discard;
   }
   
   FragColour = colour;
}