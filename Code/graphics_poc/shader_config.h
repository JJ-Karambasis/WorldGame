#ifndef SHADER_CONFIG_H
#define SHADER_CONFIG_H

#define MAX_LIGHT_COUNT 64
#define IRRADIANCE_TEXEL_COUNT 8
#define DISTANCE_TEXEL_COUNT 16
#define RAYS_PER_PROBE 300
#define HYSTERESIS 0.97f
#define VIEW_BIAS 0.1f
#define NORMAL_BIAS 0.1f
#define PROBE_IRRADIANCE_ENCODING_GAMMA 1.0f
#define PROBE_INVERSE_IRRADIANCE_ENCODING_GAMMA (1.0f/PROBE_IRRADIANCE_ENCODING_GAMMA)
#define PROBE_DISTANCE_EXPONENT 1.0f

struct irradiance_field
{    
    ak_v3f Spacing;
    ak_f32 Padding0;
    ak_v3f Count;
    ak_f32 Padding1;
    ak_v3f BottomLeft;
    ak_f32 Padding2;    
    ak_m4f RotationTransform;    
};

#endif