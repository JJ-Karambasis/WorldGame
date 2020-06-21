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

global const char* FragmentShader_Simple = R"(
out c4 FragColor;

#ifdef HAS_TEXTURES
uniform sampler2D Texture;
in v2f FragUV;
#else
uniform c4 Color;
#endif
                                             
void main()
{
#ifdef HAS_TEXTURES    
    FragColor = texture(Texture, FragUV);
#else
    FragColor = Color;    
#endif
}

)";

global const char* FragmentShader_Lighting = R"(
out c4 FragColor;

in v3f ViewP;
in v3f ViewN;

#ifdef HAS_TEXTURES
in v2f FragUV;
#endif

#ifdef DIFFUSE_COLOR
uniform c4 DiffuseColor;
#endif

#ifdef DIFFUSE_TEXTURE
uniform sampler2D DiffuseTexture;
#endif

#ifdef SPECULAR_COLOR
uniform c4 SpecularColor;
#endif

#ifdef SPECULAR_TEXTURE
uniform sampler2D SpecularTexture;
#endif

#ifdef SPECULAR_SHININESS
uniform i32 Shininess;
#endif

#define LAMBERTIAN_MODEL (defined(DIFFUSE_COLOR) || defined(DIFFUSE_TEXTURE)) && (!defined(SPECULAR_COLOR) && !defined(SPECULAR_TEXTURE) && !defined(SPECULAR_SHININESS))

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

c3 Lambertian(v3f N, v3f L, v3f SurfaceColor, v3f LightColor)
{
    c3 Result = max(dot(N, L), 0.0f)*SurfaceColor*LightColor;
    return Result;
}

brdf BlinnPhong(v3f N, v3f L, v3f V, c3 LightColor, c3 SurfaceColor, c3 SpecularColor, f32 Shininess)
{
    brdf Result;

    Result.Diffuse = Lambertian(N, L, SurfaceColor, LightColor);

    v3f H = normalize(L+V);    
    f32 NDotH = max(dot(N, H), 0.0f);
    Result.Specular = pow(NDotH, Shininess)*SpecularColor*LightColor;

    return Result;
}

void main()
{
    v3f V = -normalize(ViewP);
    v3f N = normalize(ViewN);

#ifdef DIFFUSE_COLOR
    c3 SurfaceColor = DiffuseColor.rgb;
#endif

#ifdef DIFFUSE_TEXTURE
    c3 SurfaceColor = texture(DiffuseTexture, FragUV).rgb;
#endif

#ifdef SPECULAR_COLOR
    c3 Specular = SpecularColor.rgb; 
#endif

#ifdef SPECULAR_TEXTURE
    c3 Specular = texture(SpecularTexture, FragUV).rgb;
#endif

    c3 FinalColor = c3(0, 0, 0);
    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DirectionalLightCount; DirectionalLightIndex++)
    {
        directional_light DirectionalLight = DirectionalLights[DirectionalLightIndex];
        v3f L = -DirectionalLight.Direction.xyz;

#if LAMBERTIAN_MODEL
        c3 LambertianColor = Lambertian(N, L, DirectionalLight.Color.rgb, SurfaceColor);
        FinalColor += LambertianColor;
#else
        brdf BRDF = BlinnPhong(N, L, V, DirectionalLight.Color.rgb, SurfaceColor, Specular, Shininess);
        FinalColor += (BRDF.Diffuse+BRDF.Specular);    
#endif
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

#if LAMBERTIAN_MODEL
        c3 LambertianColor = Lambertian(N, L, LightColor, SurfaceColor);
        FinalColor += LambertianColor;
#else
        brdf BRDF = BlinnPhong(N, L, V, LightColor, SurfaceColor, Specular, Shininess);
        FinalColor += (BRDF.Diffuse+BRDF.Specular);    
#endif        
    }

    c3 Ambient = 0.03*SurfaceColor.xyz;
    FinalColor += Ambient;
    FragColor = c4(FinalColor, 1.0f);
}

)";