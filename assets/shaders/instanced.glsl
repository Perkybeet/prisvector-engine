#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;

// Instance transform matrix (mat4 = 4 vec4 attributes)
layout(location = 2) in mat4 a_InstanceTransform;

uniform mat4 u_ViewProjection;

out vec3 v_Color;

void main() {
    v_Color = a_Color;
    gl_Position = u_ViewProjection * a_InstanceTransform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

in vec3 v_Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(v_Color, 1.0);
}
