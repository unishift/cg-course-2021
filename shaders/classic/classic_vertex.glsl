#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 texture_coordinates;

uniform mat4 transform;
uniform mat4 depth_transform;

out vec2 texture_coords;
out vec4 depth_coords;

void main() {
    gl_Position = transform * vec4(vertex, 1.0f);
    texture_coords = texture_coordinates;

    depth_coords = depth_transform * vec4(vertex, 1.0f);
}
