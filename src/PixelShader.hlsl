Texture2D shaderTexture;
SamplerState SampleType;

float4 main(float4 position : SV_Position, float4 color : COLOR, float2 texcoord : TEXCOORD) : SV_TARGET0
{
    float4 output;
    output = shaderTexture.Sample(SampleType, texcoord);
    output = output * color;

    return output;
}