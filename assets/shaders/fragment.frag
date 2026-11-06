#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;

void main() {
   float y = texture(texY, TexCoord).r;

   FragColor = vec4(y, y, y, 1.0);
}