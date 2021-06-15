#version 330 core

in vec2 texture_coords;
in vec4 depth_coords;

out vec4 color;

uniform vec4 diffuse_color;
uniform sampler2D Texture;
uniform int use_texture;
uniform float opacity;
uniform sampler2DShadow shadow_map;

void main() {
    float visibility = texture(shadow_map, vec3(depth_coords.xy, depth_coords.z / depth_coords.w));

    if (use_texture != 0) {
        color = texture(Texture, texture_coords);
    } else {
        color = diffuse_color;
    }
    color *= 0.75 * visibility;
    color.a = opacity;
}