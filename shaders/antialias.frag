#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_pos;

layout(set = 0, binding = 2) uniform sampler2D u_samplers[];

layout(push_constant) uniform Push {
    vec2 pixel_size;
    uint input_index;
} push;

float luma(vec3 rgb) {
    return rgb.y * (0.587/0.299) + rgb.x;
}

#define MAX_SEARCH_STEPS 128

void main() {
    out_color = texture(u_samplers[push.input_index], v_pos);
    const float luma_c = luma(out_color.rgb);
    const float luma_n = luma(texture(u_samplers[push.input_index], v_pos + vec2(0.0, -1.0) * push.pixel_size).rgb);
    const float luma_s = luma(texture(u_samplers[push.input_index], v_pos + vec2(0.0, 1.0) * push.pixel_size).rgb);
    const float luma_w = luma(texture(u_samplers[push.input_index], v_pos + vec2(-1.0, 0.0) * push.pixel_size).rgb);
    const float luma_e = luma(texture(u_samplers[push.input_index], v_pos + vec2(1.0, 0.0) * push.pixel_size).rgb);

    const float range_min = min(luma_c, min(min(luma_n, luma_w), min(luma_s, luma_e)));
    const float range_max = max(luma_c, max(max(luma_n, luma_w), max(luma_s, luma_e)));
    const float range = range_max - range_min;
    if (range < max(0.0625, range_max * 0.125))
        return;

    const vec3 rgb_nw = texture(u_samplers[push.input_index], v_pos + vec2(-1.0, -1.0) * push.pixel_size).rgb;
    const vec3 rgb_ne = texture(u_samplers[push.input_index], v_pos + vec2(1.0, -1.0) * push.pixel_size).rgb;
    const vec3 rgb_sw = texture(u_samplers[push.input_index], v_pos + vec2(-1.0, 1.0) * push.pixel_size).rgb;
    const vec3 rgb_se = texture(u_samplers[push.input_index], v_pos + vec2(1.0, 1.0) * push.pixel_size).rgb;
    const float luma_nw = luma(rgb_nw);
    const float luma_ne = luma(rgb_ne);
    const float luma_sw = luma(rgb_sw);
    const float luma_se = luma(rgb_se);

    const float vert_edge =
        abs((0.25 * luma_nw) + (-0.5 * luma_n) + (0.25 * luma_ne)) +
        abs((0.50 * luma_w ) + (-1.0 * luma_c) + (0.50 * luma_e )) +
        abs((0.25 * luma_sw) + (-0.5 * luma_s) + (0.25 * luma_se));
    const float horiz_edge =
        abs((0.25 * luma_nw) + (-0.5 * luma_w) + (0.25 * luma_sw)) +
        abs((0.50 * luma_n ) + (-1.0 * luma_c) + (0.50 * luma_s )) +
        abs((0.25 * luma_ne) + (-0.5 * luma_e) + (0.25 * luma_se));
    const bool is_horiz = horiz_edge >= vert_edge;

    const vec2 other_dir = is_horiz ? vec2(0.0, push.pixel_size.y) : vec2(push.pixel_size.x, 0.0);

    const float diff_down = abs(luma(texture(u_samplers[push.input_index], v_pos - other_dir).rgb) - luma_c);
    const float diff_up = abs(luma(texture(u_samplers[push.input_index], v_pos + other_dir).rgb) - luma_c);
    const float gradient = 0.25 * max(diff_down, diff_up);
    const vec2 other = diff_down > diff_up ? -other_dir : other_dir;

    const float luma_other = luma(texture(u_samplers[push.input_index], v_pos + other).rgb);
    const float avg_luma = (luma_c + luma_other) / 2.0;

    if (luma_c > avg_luma)
        return;

    const vec2 end_dir = is_horiz ? vec2(push.pixel_size.x, 0.0) : vec2(0.0, push.pixel_size.y);

    vec2 begin = v_pos;
    vec2 end = v_pos;

    for (int i = 0; i < MAX_SEARCH_STEPS; ++i) {
        begin -= end_dir;
        const float begin_avg_luma = luma(texture(u_samplers[push.input_index], begin + 0.5 * other).rgb);
        if (abs(begin_avg_luma - avg_luma) >= gradient)
            break;
    }
    for (int i = 0; i < MAX_SEARCH_STEPS; ++i) {
        end += end_dir;
        const float end_avg_luma = luma(texture(u_samplers[push.input_index], end + 0.5 * other).rgb);
        if (abs(end_avg_luma - avg_luma) >= gradient)
            break;
    }

    const float luma_begin = luma(texture(u_samplers[push.input_index], begin).rgb);
    const float luma_end = luma(texture(u_samplers[push.input_index], end).rgb);
    const bool is_begin_same = abs(luma_begin - luma_c) < abs(luma_other - luma_c) * 0.5;
    const bool is_end_same = abs(luma_end - luma_c) < abs(luma_other - luma_c) * 0.5;

    const float edge_length = is_horiz ? (end.x - begin.x) : (end.y - begin.y);
    const float offset_length = (is_horiz ? (v_pos.x - begin.x) : (v_pos.y - begin.y)) / edge_length;

    vec2 offset = other;
    if (is_begin_same && !is_end_same)
        offset *= offset_length;
    else if (!is_begin_same && is_end_same)
        offset *= 1.0 - offset_length;
    else
        offset *= 0.5;

    out_color = texture(u_samplers[push.input_index], v_pos + offset);
    return;
}

