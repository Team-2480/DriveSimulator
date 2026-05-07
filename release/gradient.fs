#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// NOTE: Add your custom variables here

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    float alpha = 1.0 - (fragPosition.y / 4.0);

    // NOTE: Implement here your fragment shader code

    gl_FragColor = vec4(0.0, 0.89411764705, 0.18823529411, alpha);
}