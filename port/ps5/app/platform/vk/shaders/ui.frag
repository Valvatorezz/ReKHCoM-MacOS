// ReKHCoM - the interface drawn by the GPU: the texture times the vertex colour.
// A draw whose u coordinates are offset by 2 asks for pixel art: every texel stays a crisp block and
// only the seams between blocks are blended, so uneven scaling does not shimmer. An offset of 4 asks
// for the same through the MMPX 3.2 filter without edge blending, an offset of 6 with it.
// SPDX-License-Identifier: GPL-3.0-or-later
// Compiled into shaders_ui_spv.h by tools/make-ui-shaders.sh.
#version 450
#extension GL_GOOGLE_include_directive : require

layout(set = 0, binding = 0) uniform sampler2D picture;
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_colour;
layout(location = 0) out vec4 out_color;

#include "mmpx32.glsl"

void main()
{
    vec2 uv = in_uv;
    vec2 size = vec2(textureSize(picture, 0));
    vec2 step_texels = fwidth(vec2(fract(in_uv.x * 0.25) * 4.0, in_uv.y) * size); // texels per screen pixel
    if (uv.x > 3.5)
    {
        bool blend = uv.x > 5.5; // MMPX 3.2 with edge blending; otherwise without
        vec4 colour;
        float alpha;
        uv.x -= blend ? 6.0 : 4.0;
        vec2 coords = uv * size;
        MMPXCore(coords, colour, alpha);
        if (blend)
        {
            // the edges of a shape on a transparent background are blended with the four texels around
            vec2 bc = coords - vec2(0.5);
            vec2 b0 = floor(bc);
            vec2 f = bc - b0;
            vec4 t00 = mmpxFetch(b0);
            vec4 t10 = mmpxFetch(b0 + vec2(1.0, 0.0));
            vec4 t01 = mmpxFetch(b0 + vec2(0.0, 1.0));
            vec4 t11 = mmpxFetch(b0 + vec2(1.0, 1.0));
            float w00 = (1.0 - f.x) * (1.0 - f.y);
            float w10 = f.x * (1.0 - f.y);
            float w01 = (1.0 - f.x) * f.y;
            float w11 = f.x * f.y;
            bool n00 = any(notEqual(t00, vec4(0.0)));
            bool n10 = any(notEqual(t10, vec4(0.0)));
            bool n01 = any(notEqual(t01, vec4(0.0)));
            bool n11 = any(notEqual(t11, vec4(0.0)));
            float a = (n00 ? w00 : 0.0) + (n10 ? w10 : 0.0) + (n01 ? w01 : 0.0) + (n11 ? w11 : 0.0);
            if (alpha <= 0.0 && a > 0.0)
            {
                float best = -1.0;
                if (n00 && w00 > best) { best = w00; colour = t00; }
                if (n10 && w10 > best) { best = w10; colour = t10; }
                if (n01 && w01 > best) { best = w01; colour = t01; }
                if (n11 && w11 > best) { best = w11; colour = t11; }
            }
            alpha = a;
        }
        out_color = vec4(colour.rgb, colour.a * alpha) * in_colour;
        return;
    }
    if (uv.x > 1.5)
    {
        uv.x -= 2.0;
        vec2 texel = uv * size;
        vec2 scale = max(1.0 / max(step_texels, vec2(1e-4)), vec2(1.0)); // screen pixels per texel
        vec2 base = floor(texel);
        vec2 offset = fract(texel) - 0.5;
        vec2 region = 0.5 - 0.5 / scale;
        vec2 f = (offset - clamp(offset, -region, region)) * scale + 0.5;
        uv = (base + f) / size;
    }
    out_color = texture(picture, uv) * in_colour;
}
