#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;

out vec2 v_TexCoords;

// Viewport in normalized coordinates: x, y (bottom-left), width, height
uniform vec4 u_Viewport;

void main() {
    // Transform vertex position to viewport location
    // Input quad is -1 to 1, we need to map it to viewport bounds
    vec2 pos = a_Position.xy * 0.5 + 0.5;  // 0 to 1
    pos = pos * u_Viewport.zw + u_Viewport.xy;  // Scale and translate
    pos = pos * 2.0 - 1.0;  // Back to -1 to 1

    gl_Position = vec4(pos, 0.0, 1.0);
    v_TexCoords = a_TexCoords;
}

#type fragment
#version 450 core

in vec2 v_TexCoords;
out vec4 FragColor;

// Different texture types we can display
uniform sampler2DArray u_DepthArray;  // CSM cascades
uniform sampler2D u_Texture2D;         // Regular 2D texture
uniform sampler2D u_DepthTexture;      // Single depth texture

uniform int u_Mode;       // 0=CSM depth array, 1=color texture, 2=single depth, 3=GBuffer normal
uniform int u_Layer;      // Array layer for CSM
uniform float u_NearPlane;
uniform float u_FarPlane;

// Linearize depth for better visualization
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;  // Back to NDC
    return (2.0 * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z * (u_FarPlane - u_NearPlane));
}

void main() {
    vec3 color;

    if (u_Mode == 0) {
        // CSM Depth Array - visualize cascade
        float depth = texture(u_DepthArray, vec3(v_TexCoords, float(u_Layer))).r;
        // Show depth as grayscale (closer = darker, farther = lighter)
        color = vec3(depth);
    }
    else if (u_Mode == 1) {
        // Regular color texture
        color = texture(u_Texture2D, v_TexCoords).rgb;
    }
    else if (u_Mode == 2) {
        // Single depth texture
        float depth = texture(u_DepthTexture, v_TexCoords).r;
        color = vec3(depth);
    }
    else if (u_Mode == 3) {
        // GBuffer normals (decode from 0-1 to -1 to 1, then back to 0-1 for display)
        vec3 normal = texture(u_Texture2D, v_TexCoords).rgb;
        color = normal * 0.5 + 0.5;  // Map -1..1 to 0..1 for visualization
    }
    else {
        color = vec3(1.0, 0.0, 1.0);  // Magenta for unknown mode
    }

    FragColor = vec4(color, 1.0);
}
