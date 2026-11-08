struct VertexInput {
    float2 pos;
    float4 color;
    float2 texCoord;
};

struct VertexOutput {
    float4 color;
    float2 texCoord;
    float4 position : SV_Position;
};

struct PushConstant{
    float4 color;
    bool hasTexture;
};

[[vk::push_constant]] ConstantBuffer<PushConstant> pc;
[[vk::binding(0, 0)]] Sampler2D texSampler;

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    output.color = input.color;
    output.texCoord = input.texCoord;
    output.position = float4(input.pos, 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float4 color = pc.color;
    if (pc.hasTexture){
        color = texSampler.Sample(inVert.texCoord);
    }
    return color;
}