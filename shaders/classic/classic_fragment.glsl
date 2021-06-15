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

vec2 poisson_coeffs[4] = vec2[](
    vec2(-0.94201624,   -0.39906216),
    vec2(0.94558609,    -0.76890725),
    vec2(-0.094184101,  -0.92938870),
    vec2(0.34495938,     0.29387760)
);

void main() {
    float bias = 0.005f;
    float visibility = 1.0f;

    for (int i = 0; i < 4; i++) {
        visibility -= 0.2f * (1.f - texture(shadow_map, vec3(depth_coords.xy + poisson_coeffs[i] / 1000.f, (depth_coords.z - bias) / depth_coords.w)));
    }

    vec3 n = normalize(normal_transformed);
    vec3 l = normalize(light_transformed);
    float cos_theta = clamp(dot(n, l), 0.f, 1.f);

    if (use_texture != 0) {
        color = texture(Texture, texture_coords);
    } else {
        color = diffuse_color;
    }
    color *= visibility * cos_theta;
    color += 0.1f;
    color.a = opacity;
}