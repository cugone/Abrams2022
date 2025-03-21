
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

cbuffer rounded_cb : register(b3) {
    float4 g_fill_exp_padding2;
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
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);

ps_in_t VertexFunction(vs_in_t input_vertex) {
    ps_in_t output;

    float4 local = float4(input_vertex.position.xy, 0.0f, 1.0f);
    float4 world = mul(local, g_MODEL);
    float4 view = mul(world, g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input_vertex.color;
    output.uv = input_vertex.uv;

    return output;
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0 {
    bool should_fill = g_fill_exp_padding2.x > 0.0f;
    float exp = g_fill_exp_padding2.y;
    float u = input_pixel.uv.x;
    float v = input_pixel.uv.y;
    float abs_x = abs(1.0f - u / 0.5f);
    float abs_y = abs(1.0f - v / 0.5f);
    float pixel_x = pow(abs_x, exp);
    float pixel_y = pow(abs_y, exp);
    float final_pixel = pixel_x + pixel_y - 1.0f;
    clip(-sign(final_pixel));
    if(!should_fill) {
        clip(final_pixel < 1.0f ? -1 : 1);
    }
    float4 albedo = tDiffuse.Sample(sSampler, input_pixel.uv);
    return albedo * input_pixel.color;
}
