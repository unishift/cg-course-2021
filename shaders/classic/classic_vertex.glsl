#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 texture_coordinates;
layout(location = 2) in vec3 normal;

uniform mat4 transform;
uniform mat4 depth_transform;
uniform vec3 light_direction;

out vec2 texture_coords;
out vec4 depth_coords;
out vec3 normal_transformed;
out vec3 light_transformed;

void main() {
    gl_Position = transform * vec4(vertex, 1.0f);
    texture_coords = texture_coordinates;

    depth_coords = depth_transform * vec4(vertex, 1.0f);

    normal_transformed = (transform * vec4(normal, 0.f)).xyz;
    light_transformed = (transform * vec4(light_direction, 0.f)).xyz;
}
