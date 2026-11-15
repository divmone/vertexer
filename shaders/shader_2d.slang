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
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4 color;
    bool hasTexture;
};

[[vk::binding(0, 0)]] Sampler2D texSampler;

[shader("vertex")]
VertexOutput vertMain(uniform PushConstant pc, VertexInput input)
{
    VertexOutput output;
    output.color = input.color;
    output.texCoord = input.texCoord;

    output.position = mul(
       pc.proj,
       mul(pc.view,
           mul(pc.model, float4(input.pos, 0.0, 1.0))
       )
    );
    return output;
}

[shader("fragment")]
float4 fragMain(uniform PushConstant pc, VertexOutput inVert) : SV_Target
{
    float4 color = pc.color;
    if (pc.hasTexture){
        color = texSampler.Sample(inVert.texCoord);
    }
    return color;
}