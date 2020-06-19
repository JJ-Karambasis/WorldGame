global const char* FragmentShader_ImGui = R"(
out c4 FinalColor;

in v2f FragUV;
in c4 FragColor;

uniform sampler2D Texture;

void main()
{
    FinalColor = FragColor * texture(Texture, FragUV.st);
}

)";

global const char* FragmentShader_Color = R"(
out c4 FragColor;
uniform c4 Color;
                                             
void main()
{
    FragColor = Color;    
}

)";

global const char* FragmentShader_Phong = R"(
out c4 FragColor;

in v3f ViewP;
in v3f ViewN;

uniform c4 SurfaceColor;
uniform c4 SpecularColor;
uniform i32 Shininess;

#define MAX_DIRECTIONAL_LIGHT_COUNT %d
#define MAX_POINT_LIGHT_COUNT %d

struct directional_light
{
    v4f Direction;
    c4 Color;
};

struct point_light
{
    v4f Position;
    c4 Color;
};

layout (std140) uniform LightBuffer
{
    directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    point_light PointLights[MAX_POINT_LIGHT_COUNT];
    i32 DirectionalLightCount;
    i32 PointLightCount;
};

 struct brdf
{
    c3 Diffuse;
    c3 Specular;
};

brdf BlinnPhong(v3f N, v3f L, v3f V, c3 LightColor, c3 SurfaceColor, c3 SpecularColor, f32 Shininess)
{
    brdf Result;

    v3f H = normalize(L+V);
    f32 NDotL = max(dot(N, L), 0.0f);
    f32 NDotH = max(dot(N, H), 0.0f);

    Result.Diffuse = NDotL*SurfaceColor*LightColor;
    Result.Specular = pow(NDotH, Shininess)*SpecularColor*LightColor;

    return Result;
}

void main()
{
    v3f V = -normalize(ViewP);
    v3f N = normalize(ViewN);

    c3 FinalColor = c3(0, 0, 0);
    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DirectionalLightCount; DirectionalLightIndex++)
    {
        directional_light DirectionalLight = DirectionalLights[DirectionalLightIndex];
        v3f L = -DirectionalLight.Direction.xyz;

        brdf BRDF = BlinnPhong(N, L, V, DirectionalLight.Color.xyz, SurfaceColor.xyz, SpecularColor.xyz, Shininess);
        FinalColor += (BRDF.Diffuse+BRDF.Specular);    
    }

    for(i32 PointLightIndex = 0; PointLightIndex < PointLightCount; PointLightIndex++)
    {
        point_light PointLight = PointLights[PointLightIndex];
        
        f32 LightRadius  = PointLight.Position.w;
        v3f L = PointLight.Position.xyz - ViewP;

        f32 DistanceFromLight = length(L);
        L /= DistanceFromLight;        
        
        f32 Numerator = clamp(1.0f - pow((DistanceFromLight/LightRadius), 4.0f), 0.0f, 1.0f);
        f32 Denominator = (DistanceFromLight)+1.0f;
        f32 Falloff = (Numerator*Numerator)/Denominator;

        c3 LightColor = PointLight.Color.xyz*Falloff;

        brdf BRDF = BlinnPhong(N, L, V, LightColor, SurfaceColor.xyz, SpecularColor.xyz, Shininess);
        FinalColor += (BRDF.Diffuse+BRDF.Specular);    
    }

    c3 Ambient = 0.03*SurfaceColor.xyz;
    FinalColor += Ambient;
    FragColor = c4(FinalColor, 1.0f);
}

)";