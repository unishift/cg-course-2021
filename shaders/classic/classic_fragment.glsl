#version 330 core

in vec2 texture_coords;
in vec4 depth_coords;
in vec3 normal_transformed;
in vec3 light_transformed;

out vec4 color;

uniform vec4 diffuse_color;
uniform sampler2D Texture;
uniform int use_texture;
uniform float opacity;
uniform sampler2DShadow shadow_map;

void main() {
    float visibility = texture(shadow_map, vec3(depth_coords.xy, depth_coords.z / depth_coords.w));

    vec3 n = normalize(normal_transformed);
    vec3 l = normalize(light_transformed);
    float cos_theta = clamp(dot(n, l), 0.f, 1.f);

    if (use_texture != 0) {
        color = texture(Texture, texture_coords);
    } else {
        color = diffuse_color;
    }
    color *= visibility * cos_theta;
    color.a = opacity;
}