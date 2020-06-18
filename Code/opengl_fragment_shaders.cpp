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

uniform c4 Color;

#define MAX_DIRECTIONAL_LIGHT_COUNT %d

struct directional_light
{
    v4f Direction;
    c4 Color;
};

layout (std140) uniform LightBuffer
{
    directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    i32 DirectionalLightCount;
};

void main()
{
    v3f V = -ViewP;
    v3f N = normalize(ViewN);

    c3 FinalColor = c3(0, 0, 0);
    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DirectionalLightCount; DirectionalLightIndex++)
    {
        directional_light DirectionalLight = DirectionalLights[DirectionalLightIndex];
        v3f L = -DirectionalLight.Direction.xyz;
        v3f H = normalize(L+V);

        i32 SpecularStrength = 8;
    
        f32 NDotL = max(dot(N, L), 0.0f);
        f32 NDotH = max(dot(N, H), 0.0f);
    
        c3 Diffuse = c3(Color)*NDotL;
        c3 Specular = pow(NDotH, SpecularStrength)*DirectionalLight.Color.xyz*0.5f;

        FinalColor += (Diffuse+Specular);    
    }

    f32 Ambient = 0.03f;
    FinalColor += Ambient;
    FragColor = c4(FinalColor, 1.0f);
}

)";