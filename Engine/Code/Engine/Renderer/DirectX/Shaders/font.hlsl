
cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

SamplerState sSampler : register(s0);
Texture2D<float4> tDiffuse    : register(t0);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;
    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {

    float2 uv = input_pixel.uv;
    float3 color = tDiffuse.Sample(sSampler, uv).rgb;
    float3 luminance = float3(0.299f, 0.587f, 0.114f);
    float alpha = dot(color, luminance);
    const float cutoff = 0.01f;
    if(alpha < cutoff) {
        discard;
    }
    float3 tinted_color = color * input_pixel.color.rgb;
    float tinted_alpha = alpha * input_pixel.color.a;
    float3 final_color = tinted_color;
    float final_alpha = tinted_alpha;

    float4 final_pixel = float4(final_color, final_alpha);
    return final_pixel;
}
