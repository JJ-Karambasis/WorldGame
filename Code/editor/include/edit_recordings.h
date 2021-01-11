#ifndef EDIT_RECORDINGS_H
#define EDIT_RECORDINGS_H


enum edit_entry_type
{
    EDIT_ENTRY_TYPE_CREATE,
    EDIT_ENTRY_TYPE_DELETE, 
    EDIT_ENTRY_TYPE_TRANSFORM,
    EDIT_ENTRY_TYPE_PROPERTIES,
    EDIT_ENTRY_TYPE_RENAME
};

struct dev_entity_pair
{
    ak_u32 Count;
    dev_entity Entity[2];
};

struct dev_point_light_pair
{
    ak_u32 Count;
    dev_point_light PointLight[2];
};

struct create_delete_edit_entry
{
    object_type Type;
    ak_u32 WorldIndex;
    
    union
    {
        dev_entity_pair EntityPair;
        dev_point_light_pair PointLightPair;
    };
};

struct transform_edit_entry
{
    object Object;
    ak_u32 WorldIndex;
    ak_v3f TranslationA;
    ak_v3f ScaleA;
    ak_quatf OrientationA;
    ak_v3f TranslationB;
    ak_v3f ScaleB;
    ak_quatf OrientationB;
};

struct property_edit_entry
{
    object_type Type;
    ak_u32 WorldIndex;
    
    union
    {
        dev_entity Entities[2];
        dev_point_light PointLights[2];
    };
};

struct rename_edit_entry
{
    object_type Type;
    ak_u32 WorldIndex;
    ak_string OldName;
    ak_string NewName;
};

struct edit_entry
{
    edit_entry_type Type;
    union
    {
        create_delete_edit_entry CreateDelete;
        transform_edit_entry Transform;
        property_edit_entry Properties;
        rename_edit_entry Rename;
    };
};

struct edit_recordings
{
    ak_array<edit_entry> UndoStack;
    ak_array<edit_entry> RedoStack;
    
    void ClearUndo();
    void ClearRedo();
    void Clear();
    
    void PushCreateEntry(ak_u32 WorldIndex, dev_entity* Entity);
    void PushCreateEntry(ak_u32 WorldIndex, dev_entity* EntityA, dev_entity* EntityB);
    void PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLight);
    void PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLightA, dev_point_light* PointLightB);
    
    void PushDeleteEntry(ak_u32 WorldIndex, dev_entity* Entity);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_entity* EntityA, dev_entity* EntityB);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_point_light* PointLight);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_point_light* PointLightA, dev_point_light* PointLightB);
    
    void PushTransformEntry(ak_u32 WorldIndex, object Object, ak_v3f TranslationA, ak_v3f ScaleA, ak_quatf OrientationA, 
                            ak_v3f TranslationB = AK_V3<ak_f32>(), ak_v3f ScaleB = AK_V3<ak_f32>(), ak_quatf OrientationB = AK_IdentityQuat<ak_f32>());
    
    void PushPropertyEntry(ak_u32 WorldIndex, dev_entity* OldState, dev_entity* NewState);
    void PushPropertyEntry(ak_u32 WorldIndex, dev_point_light* OldState, dev_point_light* NewState);
    
    void PushRenameEntry(ak_u32 WorldIndex, object_type Type, ak_string OldName, ak_string NewName);
    
    void Undo(editor* Editor);
    void Redo(editor* Editor);
    
};

#endif
