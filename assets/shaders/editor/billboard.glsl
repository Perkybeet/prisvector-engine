#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform vec3 u_WorldPosition;
uniform float u_Size;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

out vec2 v_TexCoord;

void main() {
    // Billboard: always face the camera
    vec3 vertexPos = u_WorldPosition
        + u_CameraRight * a_Position.x * u_Size
        + u_CameraUp * a_Position.y * u_Size;

    gl_Position = u_ViewProjection * vec4(vertexPos, 1.0);
    v_TexCoord = a_TexCoord;
}

#type fragment
#version 450 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform vec4 u_Color;
uniform int u_IconType;  // 1=Empty, 2=PointLight, 3=SpotLight, 4=DirLight, 5=AmbientLight

// Draw procedural icon shapes based on type
void main() {
    vec2 uv = v_TexCoord * 2.0 - 1.0;  // -1 to 1
    float dist = length(uv);

    float alpha = 0.0;

    if (u_IconType == 1) {
        // Empty: cube outline
        vec2 absUV = abs(uv);
        float boxDist = max(absUV.x, absUV.y);
        float boxLine = smoothstep(0.7, 0.75, boxDist) - smoothstep(0.8, 0.85, boxDist);
        // Cross in the middle
        float crossX = step(absUV.x, 0.1) * step(absUV.y, 0.4);
        float crossY = step(absUV.y, 0.1) * step(absUV.x, 0.4);
        alpha = max(boxLine, (crossX + crossY) * 0.8);
    }
    else if (u_IconType == 2) {
        // Point light: filled circle with glow
        float innerCircle = 1.0 - smoothstep(0.3, 0.35, dist);
        float outerGlow = 1.0 - smoothstep(0.35, 0.8, dist);
        alpha = innerCircle + outerGlow * 0.4;
    }
    else if (u_IconType == 3) {
        // Spot light: cone shape (triangle pointing down)
        float cone = step(uv.y, 0.5) * step(-0.5, uv.y);
        float coneWidth = (0.5 - uv.y) * 0.8;
        cone *= step(-coneWidth, uv.x) * step(uv.x, coneWidth);
        // Bulb at top
        float bulb = 1.0 - smoothstep(0.2, 0.25, length(uv - vec2(0.0, 0.5)));
        alpha = max(cone * 0.9, bulb);
    }
    else if (u_IconType == 4) {
        // Directional light: sun with rays
        float sunCore = 1.0 - smoothstep(0.25, 0.3, dist);
        // Rays
        float angle = atan(uv.y, uv.x);
        float rays = sin(angle * 8.0) * 0.5 + 0.5;
        float rayMask = step(0.3, dist) * (1.0 - smoothstep(0.5, 0.7, dist));
        alpha = sunCore + rays * rayMask * 0.7;
    }
    else if (u_IconType == 5) {
        // Ambient light: circle with outward arrows
        float ring = smoothstep(0.4, 0.45, dist) - smoothstep(0.5, 0.55, dist);
        float innerDot = 1.0 - smoothstep(0.15, 0.2, dist);
        alpha = ring * 0.8 + innerDot;
    }
    else {
        // Fallback: simple circle
        alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    }

    if (alpha < 0.01) discard;

    // Add dark outline for better visibility
    float outline = 0.0;
    float outlineDist = dist;
    if (u_IconType == 1) {
        vec2 absUV = abs(uv);
        outlineDist = max(absUV.x, absUV.y);
    }
    outline = smoothstep(0.85, 0.9, outlineDist) * (1.0 - smoothstep(0.9, 0.95, outlineDist));

    vec3 finalColor = mix(u_Color.rgb, vec3(0.1), outline * 0.8);
    FragColor = vec4(finalColor, alpha * u_Color.a);
}
