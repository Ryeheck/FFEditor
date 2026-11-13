#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;

void main() {
   float y = texture(texY, TexCoord).r;
   float u = texture(texU, TexCoord).r;
   float v = texture(texV, TexCoord).r;

   y = (y * 255.0 - 16.0) / 219.0; // [16; 235]; [black; white]
   u = (u * 255.0 - 128.0) / 224.0; // [16; 240];
   v = (v * 255.0 - 128.0) / 224.0; // [16; 240];

   float r = y + (1.5748 * v);
   float g = y - (0.1873 * u) - (0.4681 * v);
   float b = y + (1.8556 * u);

   FragColor = vec4(r, g, b, 1.0);
}