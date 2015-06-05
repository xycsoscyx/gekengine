struct WorldVertex
{
    float4 position : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

struct ViewVertex
{
    float4 position : SV_POSITION;
    float4 viewposition : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float3 viewnormal : NORMAL0;
    float4 color : COLOR0;
};

WorldVertex getWorldVertex(in PluginVertex pluginVertex);

ViewVertex mainVertexProgram(in PluginVertex pluginVertex)
{
    WorldVertex worldVertex = getWorldVertex(pluginVertex);

    ViewVertex viewVertex;
    viewVertex.viewposition = mul(Camera::viewMatrix, worldVertex.position);
    viewVertex.position = mul(Camera::projectionMatrix, viewVertex.viewposition);
    viewVertex.texcoord = worldVertex.texcoord;
    viewVertex.viewnormal = mul((float3x3)Camera::viewMatrix, worldVertex.normal);
    viewVertex.color = worldVertex.color;
    return viewVertex;
}
