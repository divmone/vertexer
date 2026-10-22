struct VertexOutput {
    float3 worldPos;
    float3 normal;
    float2 texCoord;
    float4 sv_position : SV_Position;
};

struct UBObject {
    float3 lightPos;
    float3 lightColor;
    float3 ambient;
    float3 cameraPos;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UBObject> ubo;

[[vk::binding(0, 1)]] Sampler2D texSampler;

struct PushConstant{
    float4x4 model;
    float4 color;
    bool hasTexture;
};

float3x3 inverse3x3(float3x3 m) {
    float det = dot(m[0], cross(m[1], m[2]));
    float3x3 adj;
    adj[0] = cross(m[1], m[2]);
    adj[1] = cross(m[2], m[0]);
    adj[2] = cross(m[0], m[1]);
    return transpose(adj) / det;
}

[shader("vertex")]
VertexOutput vertMain(uniform PushConstant pushConstant, VertexInput input) {
    VertexOutput output;
    output.sv_position = mul(ubo.proj, mul(ubo.view, mul(pushConstant.model, float4(input.pos, 1.0))));
    output.texCoord = input.texCoord;
    float3x3 normalMatrix = transpose(inverse3x3((float3x3)pushConstant.model));
    output.normal = normalize(mul(normalMatrix, input.normal));
    output.worldPos = mul(pushConstant.model, float4(input.pos, 1.0)).xyz;
    return output;
}

[shader("fragment")]
float4 fragMain(uniform PushConstant pushConstant, VertexOutput inVert) : SV_Target
{
    float3 norm = normalize(inVert.normal);
    float3 lightDir = normalize(ubo.lightPos - inVert.worldPos);
    float3 viewDir = normalize(ubo.cameraPos - inVert.worldPos);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * ubo.lightColor;

    float specAngle = max(dot(norm, halfwayDir), 0.0);
    float spec = pow(specAngle, 32.0);
    float3 specular = spec * ubo.lightColor;

    float4 albedo = pushConstant.color;

    if (pushConstant.hasTexture){
        albedo *= texSampler.Sample(inVert.texCoord);
        if (albedo.a < 0.5){
            discard;
        }
    }

    float3 color = (ubo.ambient + diffuse) * albedo.xyz + specular;
    return float4(color, albedo.a);
}