#ifndef SIM_ENTITY_H
#define SIM_ENTITY_H

enum sim_entity_type
{
    SIM_ENTITY_TYPE_SIM_ENTITY,
    SIM_ENTITY_TYPE_RIGID_BODY
};

struct sim_entity_id
{
    sim_entity_type Type;
    u64 ID;    
};

struct rigid_body;

struct sim_entity
{
    sim_entity_id ID;
    sqt Transform;
    collision_volume* CollisionVolumes;
    void* UserData;
    
    inline rigid_body* ToRigidBody()
    {
        if(ID.Type == SIM_ENTITY_TYPE_RIGID_BODY)
            return (rigid_body*)this;
        return NULL;
    }
    
    v3f GetMoveDelta();    
};

struct rigid_body : public sim_entity
{
    v3f Velocity;
    v3f Acceleration;    
    v3f MoveDelta;    
    v3f LinearDamping;
    v3f AngularDamping;
    v3f Force;
    v3f Torque;
    v3f AngularVelocity;
    v3f AngularAcceleration;
    v3f LocalCenterOfMass;
    v3f WorldCenterOfMass;
    m3 LocalInvInertiaTensor;
    m3 WorldInvInertiaTensor;
    f32 Restitution;
    f32 InvMass;
    
    inline void ApplyForce(v3f Direction, f32 Strength) { Force += (Direction*Strength); }    
    inline void ApplyForce(v2f Direction, f32 Strength) { Force.xy += (Direction*Strength); }    
    inline void ApplyConstantAcceleration(v3f Direction, f32 AccelerationStrength) { ApplyForce(Direction, AccelerationStrength*(1.0f/InvMass)); }
    inline void ApplyConstantAcceleration(v2f Direction, f32 AccelerationStrength) { ApplyForce(Direction, AccelerationStrength*(1.0f/InvMass)); }
    inline void ApplyGravity(f32 GravityStrength) { ApplyConstantAcceleration(-Global_WorldZAxis, GravityStrength); }
    inline void ClearForce() { Force = {}; }
    inline void ClearTorque() { Torque = {}; }    
    inline void ApplyLinearDamp(f32 dt)  { Velocity        *= (1.0f / (1.0f + dt*LinearDamping)); }
    inline void ApplyAngularDamp(f32 dt) { AngularVelocity *= (1.0f / (1.0f + dt*AngularDamping)); }
    inline m3 GetWorldInvInertiaTensor() 
    { 
        m3 OrientationMatrix = ToMatrix3(Transform.Orientation);    
        return Transpose(OrientationMatrix)*LocalInvInertiaTensor*OrientationMatrix;            
    }
};

inline v3f sim_entity::GetMoveDelta()
{
    rigid_body* RigidBody = ToRigidBody();
    v3f Result = RigidBody ? RigidBody->MoveDelta : V3();
    return Result;
}

#endif