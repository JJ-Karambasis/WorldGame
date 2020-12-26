#ifndef SELECTOR_H
#define SELECTOR_H

struct selector
{
    ray RayCast;
    ak_u32 WorldIndex;
    
    void Begin(editor* Editor, view_settings* ViewSettings, ak_v2i Resolution);
    
    selected_object* GetSelectedObject();
    void SetTransformMode(selector_transform_mode Mode);
    selector_transform_mode GetTransformMode();
    ak_v3f CalcDiff();
    ak_quatf CalcOrientationDiff(ak_v3f Diff);
};

#endif
