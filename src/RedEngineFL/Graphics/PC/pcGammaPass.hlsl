sampler2D sScene : register(s0);
sampler2D sLut : register(s1);

float4 main(float2 vUv : TEXCOORD0) : COLOR
{
    float4 vColor = tex2D(sScene, vUv);
    float3 vIndex = vColor.rgb * (255.0 / 256.0) + (0.5 / 256.0);
    float fR = tex2D(sLut, float2(vIndex.r, 0.5)).r;
    float fG = tex2D(sLut, float2(vIndex.g, 0.5)).g;
    float fB = tex2D(sLut, float2(vIndex.b, 0.5)).b;
    return float4(fR, fG, fB, vColor.a);
}
