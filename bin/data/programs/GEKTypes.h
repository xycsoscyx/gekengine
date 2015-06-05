#ifdef _MODE_FORWARD
    struct InputPixel
    {
        float4 position                     : SV_POSITION;
        float4 viewposition                 : TEXCOORD0;
        float2 texcoord                     : TEXCOORD1;
        float3 viewnormal                   : NORMAL0;
        float4 color                        : COLOR0;
        bool   frontface                    : SV_ISFRONTFACE;
    };
#else
    struct InputPixel
    {
        float4 position                     : SV_POSITION;
        float2 texcoord                     : TEXCOORD0;
    };
#endif