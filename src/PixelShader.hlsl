Texture2D shaderTexture;

SamplerState SampleType
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float4 main(float4 position : SV_Position, float4 color : COLOR, float2 texcoord : TEXCOORD) : SV_TARGET0
{
    return shaderTexture.Sample(SampleType, texcoord);

}