#ifndef SIM_ENTITY_H
#define SIM_ENTITY_H

enum sim_entity_type
{
    SIM_ENTITY_TYPE_SIM_ENTITY,
    SIM_ENTITY_TYPE_RIGID_BODY
};

struct sim_entity_id
{
    ak_u64 ID;
    sim_entity_type Type;
};

struct rigid_body;
struct sim_entity
{
    sim_entity_id ID;    
    ak_sqtf Transform;
    ak_u64 CollisionVolumeID;    
    void* UserData;
    
    inline rigid_body* ToRigidBody()
    {
        if(ID.Type == SIM_ENTITY_TYPE_RIGID_BODY)
            return (rigid_body*)this;
        return NULL;
    }
    
    ak_v3f GetMoveDelta();    
};

struct rigid_body : public sim_entity
{    
    ak_v3f Velocity;
    ak_v3f Acceleration;    
    ak_v3f MoveDelta;    
    ak_v3f LinearDamping;
    ak_v3f AngularDamping;
    ak_v3f Force;
    ak_v3f Torque;
    ak_v3f AngularVelocity;
    ak_v3f AngularAcceleration;
    ak_v3f LocalCenterOfMass;
    ak_v3f WorldCenterOfMass;
    ak_m3f LocalInvInertiaTensor;
    ak_m3f WorldInvInertiaTensor;
    ak_f32 Restitution;
    ak_f32 InvMass;
    
    inline ak_f32 GetMoveDistance() { return AK_Magnitude(MoveDelta); }
    inline ak_v3f GetMoveDirection() { return MoveDelta/GetMoveDistance(); }
    
    inline void ApplyForce(ak_v3f Direction, ak_f32 Strength) { Force += (Direction*Strength); }    
    inline void ApplyForce(ak_v2f Direction, ak_f32 Strength) { Force.xy += (Direction*Strength); }    
    inline void ApplyConstantAcceleration(ak_v3f Direction, ak_f32 AccelerationStrength) { ApplyForce(Direction, AccelerationStrength*(1.0f/InvMass)); }
    inline void ApplyConstantAcceleration(ak_v2f Direction, ak_f32 AccelerationStrength) { ApplyForce(Direction, AccelerationStrength*(1.0f/InvMass)); }
    inline void ApplyGravity(ak_f32 GravityStrength) { ApplyConstantAcceleration(-AK_ZAxis(), GravityStrength); }
    inline void ClearForce() { Force = {}; }
    inline void ClearTorque() { Torque = {}; }    
    inline void ApplyLinearDamp(ak_f32 dt)  { Velocity        *= (1.0f / (1.0f + dt*LinearDamping)); }
    inline void ApplyAngularDamp(ak_f32 dt) { AngularVelocity *= (1.0f / (1.0f + dt*AngularDamping)); }    
    inline void IntegrateVelocity(ak_f32 dt) { Velocity += Acceleration*dt; ApplyLinearDamp(dt); }
    inline void IntegrateAngVelocity(ak_f32 dt) { AngularVelocity += AngularAcceleration*dt; ApplyAngularDamp(dt); }
    inline ak_m3f GetWorldInvInertiaTensor() 
    { 
        ak_m3f OrientationMatrix = AK_QuatToMatrix(Transform.Orientation);    
        return AK_Transpose(OrientationMatrix)*LocalInvInertiaTensor*OrientationMatrix;            
    }
};

inline ak_v3f sim_entity::GetMoveDelta()
{
    rigid_body* RigidBody = ToRigidBody();
    ak_v3f Result = RigidBody ? RigidBody->MoveDelta : AK_V3<ak_f32>();
    return Result;
}

#endif