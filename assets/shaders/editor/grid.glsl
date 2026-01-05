#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;

out vec3 v_WorldPos;

void main() {
    v_WorldPos = a_Position;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

in vec3 v_WorldPos;
out vec4 FragColor;

uniform float u_GridSize;
uniform float u_FadeDistance;
uniform vec3 u_CameraPos;

void main() {
    // Grid lines using screen-space derivatives
    vec2 coord = v_WorldPos.xz / u_GridSize;
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    float alpha = 1.0 - min(line, 1.0);

    // Fade with distance from camera
    float dist = length(v_WorldPos.xz - u_CameraPos.xz);
    float distanceFade = 1.0 - smoothstep(u_FadeDistance * 0.3, u_FadeDistance, dist);
    alpha *= distanceFade;

    // Base grid color (dark gray)
    vec3 color = vec3(0.35);

    // Major grid lines (every 10 units) - darker and more prominent
    vec2 majorCoord = v_WorldPos.xz / (u_GridSize * 10.0);
    vec2 majorGrid = abs(fract(majorCoord - 0.5) - 0.5) / fwidth(majorCoord);
    float majorLine = min(majorGrid.x, majorGrid.y);

    if (majorLine < 1.0) {
        float majorAlpha = 1.0 - min(majorLine, 1.0);
        majorAlpha *= distanceFade;
        color = vec3(0.2);
        alpha = max(alpha, majorAlpha * 0.8);
    }

    // Axis lines (X = red tint, Z = blue tint)
    float axisWidth = 0.05 * u_GridSize;
    if (abs(v_WorldPos.z) < axisWidth) {
        color = vec3(0.8, 0.2, 0.2); // X axis - red
        alpha = max(alpha, 0.6 * distanceFade);
    }
    if (abs(v_WorldPos.x) < axisWidth) {
        color = vec3(0.2, 0.2, 0.8); // Z axis - blue
        alpha = max(alpha, 0.6 * distanceFade);
    }

    // Discard if completely transparent
    if (alpha < 0.001) discard;

    FragColor = vec4(color, alpha * 0.6);
}
