void Internal__PushToUndo(edit_recordings* Recordings, edit_entry Entry)
{
    Recordings->UndoStack.Add(Entry);
    Recordings->RedoStack.Clear();
}

void edit_recordings::ClearUndo()
{
    UndoStack.Clear();
}

void edit_recordings::ClearRedo()
{
    RedoStack.Clear();
}

void edit_recordings::Clear()
{
    ClearUndo();
    ClearRedo();
}


void edit_recordings::PushCreateEntry(ak_u32 WorldIndex, dev_entity* Entity)
{
    AK_Assert(AK_StringIsNullOrEmpty(Entity->LinkName), "If object has a linked entity, please pass it into this function");
    
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_CREATE;
    Entry.CreateDelete.Type = OBJECT_TYPE_ENTITY;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.EntityPair.Count =1;
    Entry.CreateDelete.EntityPair.Entity[0] = *Entity;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushCreateEntry(ak_u32 WorldIndex, dev_entity* EntityA, dev_entity* EntityB)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_CREATE;
    Entry.CreateDelete.Type = OBJECT_TYPE_ENTITY;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.EntityPair.Entity[0] = *EntityA;
    Entry.CreateDelete.EntityPair.Entity[1] = *EntityB;
    
    if(!AK_StringIsNullOrEmpty(EntityA->LinkName))
    {
        AK_Assert(AK_StringEquals(EntityA->LinkName, EntityB->Name), "If EntityA has a linked entity, then that entity must be EntityB");
        AK_Assert(AK_StringEquals(EntityB->LinkName, EntityA->Name), "If EntityB has a linked entity, then that entity must be EntityA");
        Entry.CreateDelete.EntityPair.Count = 1;
    }
    else
    {
        AK_Assert(AK_StringIsNullOrEmpty(EntityB->LinkName), "EntityB must not have a linked entity if entity A does not have any");
        Entry.CreateDelete.EntityPair.Count = 2;
    }
    
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLight)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_CREATE;
    Entry.CreateDelete.Type = OBJECT_TYPE_LIGHT;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.PointLightPair.Count = 1;
    Entry.CreateDelete.PointLightPair.PointLight[0] = *PointLight;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLightA, dev_point_light* PointLightB)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_CREATE;
    Entry.CreateDelete.Type = OBJECT_TYPE_LIGHT;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.PointLightPair.Count = 2;
    Entry.CreateDelete.PointLightPair.PointLight[0] = *PointLightA;
    Entry.CreateDelete.PointLightPair.PointLight[1] = *PointLightB;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushDeleteEntry(ak_u32 WorldIndex, dev_entity* Entity)
{
    AK_Assert(AK_StringIsNullOrEmpty(Entity->LinkName), "If object has a linked entity, please pass it into this function");
    
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_DELETE;
    Entry.CreateDelete.Type = OBJECT_TYPE_ENTITY;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.EntityPair.Count =1;
    Entry.CreateDelete.EntityPair.Entity[0] = *Entity;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushDeleteEntry(ak_u32 WorldIndex, dev_point_light* PointLight)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_DELETE;
    Entry.CreateDelete.Type = OBJECT_TYPE_LIGHT;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.PointLightPair.Count = 1;
    Entry.CreateDelete.PointLightPair.PointLight[0] = *PointLight;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushTransformEntry(ak_u32 WorldIndex, object Object, ak_v3f Translation, ak_v3f Scale, ak_quatf Orientation, 
                                         ak_v3f Euler)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_TRANSFORM;
    Entry.Transform.Object = Object;
    Entry.Transform.WorldIndex = WorldIndex;
    Entry.Transform.Translation = Translation;
    Entry.Transform.Scale = Scale;
    Entry.Transform.Orientation = Orientation;
    Entry.Transform.Euler = Euler;
    Internal__PushToUndo(this, Entry);
}


void edit_recordings::PushPropertyEntry(ak_u32 WorldIndex, dev_entity* OldState, dev_entity* NewState)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_PROPERTIES;
    Entry.Properties.Type = OBJECT_TYPE_ENTITY;
    Entry.Properties.WorldIndex = WorldIndex;
    Entry.Properties.Entities[0] = *OldState;
    Entry.Properties.Entities[1] = *NewState;
    Internal__PushToUndo(this, Entry);
}

void edit_recordings::PushPropertyEntry(ak_u32 WorldIndex, dev_point_light* OldState, dev_point_light* NewState)
{
    edit_entry Entry = {};
    Entry.Type = EDIT_ENTRY_TYPE_PROPERTIES;
    Entry.Properties.Type = OBJECT_TYPE_LIGHT;
    Entry.Properties.WorldIndex = WorldIndex;
    Entry.Properties.PointLights[0] = *OldState;
    Entry.Properties.PointLights[1] = *NewState;
    Internal__PushToUndo(this, Entry);
}


void edit_recordings::Undo(world_management* WorldManagement)
{
    edit_entry* Entry = UndoStack.Pop(); 
    if(Entry)
    {
        switch(Entry->Type)
        {
            case EDIT_ENTRY_TYPE_CREATE:
            case EDIT_ENTRY_TYPE_DELETE:
            {
                create_delete_edit_entry* CreatedEntry = &Entry->CreateDelete;
                
                switch(CreatedEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        if(Entry->Type == EDIT_ENTRY_TYPE_CREATE)
                        {
                            WorldManagement->DeleteDevEntity(CreatedEntry->WorldIndex, CreatedEntry->EntityPair.Entity[0].Name);
                            
                            if(CreatedEntry->EntityPair.Count > 1)
                            {
                                WorldManagement->DeleteDevEntity(!CreatedEntry->WorldIndex, 
                                                                 CreatedEntry->EntityPair.Entity[1].Name);
                            }
                        }
                        else
                        {
                            WorldManagement->CopyDevEntity(CreatedEntry->EntityPair.Entity, 
                                                           CreatedEntry->WorldIndex);
                            if(CreatedEntry->EntityPair.Count > 1)
                            {
                                WorldManagement->CopyDevEntity(&CreatedEntry->EntityPair.Entity[1], 
                                                               CreatedEntry->WorldIndex);
                            }
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        if(Entry->Type == EDIT_ENTRY_TYPE_CREATE)
                        {
                            WorldManagement->DeleteDevPointLight(CreatedEntry->WorldIndex, CreatedEntry->PointLightPair.PointLight[0].Name);
                            if(CreatedEntry->PointLightPair.Count > 1)
                            {
                                WorldManagement->DeleteDevPointLight(!CreatedEntry->WorldIndex, 
                                                                     CreatedEntry->PointLightPair.PointLight[1].Name);
                            }
                        }
                        else
                        {
                            WorldManagement->CopyDevPointLight(CreatedEntry->PointLightPair.PointLight, 
                                                               CreatedEntry->WorldIndex);
                            if(CreatedEntry->PointLightPair.Count > 1)
                            {
                                WorldManagement->CopyDevPointLight(&CreatedEntry->PointLightPair.PointLight[1], 
                                                                   !CreatedEntry->WorldIndex);
                            }
                        }
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
            } break;
            
            case EDIT_ENTRY_TYPE_TRANSFORM:
            {
                transform_edit_entry* TransformEntry = &Entry->Transform;
                object* Object = &TransformEntry->Object;
                
                switch(Object->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* DevEntity = Object->GetEntity(WorldManagement, TransformEntry->WorldIndex);
                        DevEntity->Transform.Translation -= TransformEntry->Translation;
                        DevEntity->Transform.Scale -= TransformEntry->Scale;
                        DevEntity->Transform.Orientation *= AK_Conjugate(TransformEntry->Orientation);
                        DevEntity->Euler -= TransformEntry->Euler;
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        dev_point_light* DevPointLight = 
                            Object->GetPointLight(WorldManagement, TransformEntry->WorldIndex);
                        DevPointLight->Light.Position -= TransformEntry->Translation;
                    } break;
                }
                
            } break;
            
            case EDIT_ENTRY_TYPE_PROPERTIES:
            {
                property_edit_entry* PropertyEntry = &Entry->Properties;
                ak_u32 WorldIndex = PropertyEntry->WorldIndex;
                
                switch(PropertyEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        ak_u64* ID = WorldManagement->EntityTables[WorldIndex].Find(PropertyEntry->Entities[0].Name);
                        AK_Assert(ID, "Entity must be allocated for the properties to be edited");
                        dev_entity* DevEntity = WorldManagement->DevEntities[WorldIndex].Get(*ID);
                        *DevEntity = PropertyEntry->Entities[0];
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        ak_u64* ID = WorldManagement->PointLightTables[WorldIndex].Find(PropertyEntry->PointLights[0].Name);
                        AK_Assert(ID, "Point light must be allocated for the properties to be edited");
                        dev_point_light* DevPointLight = WorldManagement->DevPointLights[WorldIndex].Get(*ID);
                        *DevPointLight = PropertyEntry->PointLights[0];
                    } break;
                }
                
            } break;
            
            AK_INVALID_DEFAULT_CASE;
        }
        
        RedoStack.Add(*Entry);
    }
}

void edit_recordings::Redo(world_management* WorldManagement)
{
    edit_entry* Entry = RedoStack.Pop();
    if(Entry)
    {
        switch(Entry->Type)
        {
            case EDIT_ENTRY_TYPE_CREATE:
            case EDIT_ENTRY_TYPE_DELETE:
            {
                create_delete_edit_entry* CreatedEntry = &Entry->CreateDelete;
                
                switch(CreatedEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        if(Entry->Type == EDIT_ENTRY_TYPE_CREATE)
                        {
                            WorldManagement->CopyDevEntity(CreatedEntry->EntityPair.Entity, CreatedEntry->WorldIndex);
                            if(CreatedEntry->EntityPair.Count > 1)
                            {
                                WorldManagement->CopyDevEntity(&CreatedEntry->EntityPair.Entity[1], 
                                                               !CreatedEntry->WorldIndex);
                            }
                        }
                        else
                        {
                            WorldManagement->DeleteDevEntity(CreatedEntry->WorldIndex, CreatedEntry->EntityPair.Entity[0].Name);
                            
                            if(CreatedEntry->EntityPair.Count > 1)
                            {
                                WorldManagement->DeleteDevEntity(!CreatedEntry->WorldIndex, 
                                                                 CreatedEntry->EntityPair.Entity[1].Name);
                            }
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        if(Entry->Type == EDIT_ENTRY_TYPE_CREATE)
                        {
                            WorldManagement->CopyDevPointLight(CreatedEntry->PointLightPair.PointLight, 
                                                               CreatedEntry->WorldIndex);
                            if(CreatedEntry->PointLightPair.Count > 1)
                            {
                                WorldManagement->CopyDevPointLight(&CreatedEntry->PointLightPair.PointLight[1], 
                                                                   !CreatedEntry->WorldIndex);
                            }
                        }
                        else
                        {
                            WorldManagement->DeleteDevPointLight(CreatedEntry->WorldIndex, CreatedEntry->PointLightPair.PointLight[0].Name);
                            if(CreatedEntry->PointLightPair.Count > 1)
                            {
                                WorldManagement->DeleteDevPointLight(!CreatedEntry->WorldIndex, 
                                                                     CreatedEntry->PointLightPair.PointLight[1].Name);
                            }
                        }
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
            } break;
            
            case EDIT_ENTRY_TYPE_TRANSFORM:
            {
                transform_edit_entry* TransformEntry = &Entry->Transform;
                object* Object = &TransformEntry->Object;
                
                switch(Object->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* DevEntity = Object->GetEntity(WorldManagement, TransformEntry->WorldIndex);
                        DevEntity->Transform.Translation += TransformEntry->Translation;
                        DevEntity->Transform.Scale += TransformEntry->Scale;
                        DevEntity->Transform.Orientation *= TransformEntry->Orientation;
                        DevEntity->Euler += TransformEntry->Euler;
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        dev_point_light* DevPointLight = 
                            Object->GetPointLight(WorldManagement, TransformEntry->WorldIndex);
                        DevPointLight->Light.Position += TransformEntry->Translation;
                    } break;
                }
            } break;
            
            
            case EDIT_ENTRY_TYPE_PROPERTIES:
            {
                property_edit_entry* PropertyEntry = &Entry->Properties;
                ak_u32 WorldIndex = PropertyEntry->WorldIndex;
                
                switch(PropertyEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        ak_u64* ID = WorldManagement->EntityTables[WorldIndex].Find(PropertyEntry->Entities[0].Name);
                        AK_Assert(ID, "Entity must be allocated for the properties to be edited");
                        dev_entity* DevEntity = WorldManagement->DevEntities[WorldIndex].Get(*ID);
                        *DevEntity = PropertyEntry->Entities[1];
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        ak_u64* ID = WorldManagement->PointLightTables[WorldIndex].Find(PropertyEntry->PointLights[0].Name);
                        AK_Assert(ID, "Point light must be allocated for the properties to be edited");
                        dev_point_light* DevPointLight = WorldManagement->DevPointLights[WorldIndex].Get(*ID);
                        *DevPointLight = PropertyEntry->PointLights[1];
                    } break;
                }
                
            } break;
            
            
            AK_INVALID_DEFAULT_CASE;
        }
        
        UndoStack.Add(*Entry);
    }
}