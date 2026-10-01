// [CHOSEN, not recovered] private TrailSystem emission split. Full BF2 RendDX9 PE
// 1017B0A0/B8E0/C490/C5C0/C7B0 instructions read; no dedicated ELF renderer body.
// Trail particles are 168 bytes and use the instance's circular array, not a null-ended list.
int psmType2AdvanceParticle(void*,float*,int,float,unsigned int);

void psmType2EmissionDirection(float* output,void* descriptor,void* self)
{
    output[0]=psmRuntimeField<float>(descriptor,0x1AC)*psmRuntimeField<float>(self,0x168);
    output[1]=psmRuntimeField<float>(descriptor,0x1B0)*psmRuntimeField<float>(self,0x16C);
    output[2]=psmRuntimeField<float>(descriptor,0x1B4)*psmRuntimeField<float>(self,0x170);
}
void psmType2EmissionSphere(float* output,void* descriptor,void* self,float* particle)
{
    const float phi=static_cast<float>(psmEmissionRandomInteger()*static_cast<double>(3.7450703871400037e-7f));
    const double theta=psmEmissionRandomInteger()*static_cast<double>(1.8725351935700019e-7f);
    const float savedTheta=static_cast<float>(theta), sineTheta=static_cast<float>(::sin(theta));
    const float yRandom=static_cast<float>(psmEmissionUnit()), xRandom=static_cast<float>(psmEmissionUnit());
    const double zRadius=psmEmissionRandomInteger()*static_cast<double>(psmRuntimeField<float>(descriptor,0x1C))
        *0.000000059604644775390625;
    const double xRadius=static_cast<double>(xRandom)*psmRuntimeField<float>(descriptor,0x14);
    const double yRadius=static_cast<double>(yRandom)*psmRuntimeField<float>(descriptor,0x18);
    particle[1]=static_cast<float>(::cos(phi)*xRadius*sineTheta);
    particle[2]=static_cast<float>(::sin(phi)*yRadius*sineTheta);
    particle[3]=static_cast<float>(::cos(savedTheta)*zRadius);
    psmType2EmissionDirection(output,descriptor,self);
}
void psmType2EmissionCylinder(float* output,void* descriptor,void* self,float* particle)
{
    const double angle=(psmEmissionSymmetric()*psmRuntimeField<float>(descriptor,0x5C)
        +psmRuntimeField<float>(descriptor,0x60))*0.01745329238474369f;
    const float cosine=static_cast<float>(::cos(angle)), sine=static_cast<float>(::sin(angle));
    unsigned int axis=0;
    if(psmRuntimeField<float>(descriptor,0x18)>psmRuntimeField<float>(descriptor,0x14))axis=1;
    if(psmRuntimeField<float>(descriptor,0x1C)>psmRuntimeField<float>(descriptor,0x14+4*axis))axis=2;
    unsigned char* const bytes=static_cast<unsigned char*>(descriptor);
    const float z=static_cast<float>(psmEmissionDistribution(bytes+0x48));
    const float y=static_cast<float>(psmEmissionDistribution(bytes+0x34));
    const double x=psmEmissionDistribution(bytes+0x20);
    if(axis==0){particle[1]=static_cast<float>(x);particle[2]=y*sine;particle[3]=z*cosine;}
    else if(axis==1){particle[1]=static_cast<float>(x*sine);particle[2]=y;particle[3]=z*cosine;}
    else{particle[1]=static_cast<float>(x*sine);particle[2]=y*cosine;particle[3]=z;}
    if(psmRuntimeField<int>(descriptor,0x224)==1)
    {
        output[0]=particle[1];output[1]=particle[2];output[2]=particle[3];output[axis]=0;
        psmNormalize3(output);
    }
    else psmType2EmissionDirection(output,descriptor,self);
}
void psmType2EmissionBox(float* output,void* descriptor,void* self,float* particle)
{
    unsigned char* const bytes=static_cast<unsigned char*>(descriptor);
    const float z=static_cast<float>(psmEmissionDistribution(bytes+0x48));
    const float y=static_cast<float>(psmEmissionDistribution(bytes+0x34));
    particle[1]=static_cast<float>(psmEmissionDistribution(bytes+0x20));
    particle[2]=y;particle[3]=z;psmType2EmissionDirection(output,descriptor,self);
}

// PE1017B0A0 and its virtual thunk1017BC60. The otherwise unused third RNG draw is retained.
float* psmType2NewParticle(void* self)
{
    void* const manager=g_ParticleSystemManager;
    float* const particle=psmRuntimeField<float*>(manager,0x85E0);
    psmRuntimeField<float*>(manager,0x85E0)=particle?psmRuntimeField<float*>(particle,0):nullptr;
    if(!particle)return nullptr;
    ++psmRuntimeField<unsigned int>(self,0x138);
    const double dx=static_cast<double>(psmRuntimeField<float>(self,0xA4))-psmRuntimeField<float>(self,0x15C);
    const double dy=static_cast<double>(psmRuntimeField<float>(self,0xA8))-psmRuntimeField<float>(self,0x160);
    const double dz=static_cast<double>(psmRuntimeField<float>(self,0xAC))-psmRuntimeField<float>(self,0x164);
    const double distance=::sqrt(dx*dx+dz*dz+dy*dy)+psmRuntimeField<float>(self,0x158);
    psmRuntimeField<float>(self,0x158)=static_cast<float>(distance);particle[32]=static_cast<float>(distance);
    psmRuntimeField<Vec3>(self,0x15C)=psmRuntimeField<Vec3>(self,0xA4);
    void* descriptor=psmRuntimeField<void*>(self,0x2C);
    particle[39]=psmRuntimeField<float>(descriptor,0x70);particle[40]=static_cast<float>(1.0/particle[39]);
    particle[38]=psmRuntimeField<float>(self,0xE0);particle[30]=particle[38]*particle[40];
    particle[28]=psmRuntimeField<float>(self,0x144);
    particle[31]=psmRuntimeField<float>(self,0x148);particle[34]=psmRuntimeField<float>(self,0x148);
    if(!psmRuntimeField<unsigned int>(self,0x28))particle[34]=0;
    const float firstRandom=static_cast<float>(psmEmissionSymmetric());
    particle[29]=static_cast<float>(psmEmissionSymmetric()*psmRuntimeField<float>(descriptor,0x150));
    psmEmissionRaw();
    float direction[3]={0,0,0};
    descriptor=psmRuntimeField<void*>(self,0x2C);
    switch(psmRuntimeField<int>(descriptor,0x224))
    {
    case 0:psmType2EmissionSphere(direction,descriptor,self,particle);break;
    case 1:case 2:psmType2EmissionCylinder(direction,descriptor,self,particle);break;
    case 3:psmType2EmissionBox(direction,descriptor,self,particle);break;
    }
    const float randomX=static_cast<float>(psmEmissionSymmetric());
    const float randomY=static_cast<float>(psmEmissionSymmetric());
    const float randomZ=static_cast<float>(psmEmissionSymmetric());
    Mat4 rotation,axis,copy;
    psmEmissionRotation(rotation,randomX*psmRuntimeField<float>(descriptor,0x158),0);
    psmEmissionRotation(axis,randomY*psmRuntimeField<float>(descriptor,0x15C),1);
    copy=rotation;psmEmissionRotateMultiply(rotation,copy,axis);
    psmEmissionRotation(axis,randomZ*psmRuntimeField<float>(descriptor,0x160),2);
    copy=rotation;psmEmissionRotateMultiply(rotation,copy,axis);
    const float x=direction[0],y=direction[1],z=direction[2];
    const double rotatedZ=static_cast<double>(rotation.m[2])*x+static_cast<double>(rotation.m[6])*y+static_cast<double>(rotation.m[10])*z;
    const float rotatedY=static_cast<float>(static_cast<double>(rotation.m[1])*x+static_cast<double>(rotation.m[9])*z+static_cast<double>(rotation.m[5])*y);
    direction[0]=static_cast<float>(static_cast<double>(rotation.m[0])*x+static_cast<double>(rotation.m[8])*z+static_cast<double>(rotation.m[4])*y);
    direction[1]=rotatedY;direction[2]=static_cast<float>(rotatedZ);
    const float speed=static_cast<float>((psmRuntimeField<float>(descriptor,0x164)
        -static_cast<double>(firstRandom)*psmRuntimeField<float>(descriptor,0x154))*psmRuntimeField<float>(self,0x134));
    const float* const matrix=psmRuntimeField<Mat4>(self,0x74).m;
    if(!psmRuntimeField<unsigned char>(descriptor,0x1C0))
    {
        const float px=particle[1],py=particle[2],pz=particle[3];
        particle[3]=static_cast<float>(static_cast<double>(matrix[2])*px+static_cast<double>(matrix[10])*pz+static_cast<double>(matrix[6])*py+matrix[14]);
        particle[2]=static_cast<float>(static_cast<double>(matrix[1])*px+static_cast<double>(matrix[9])*pz+static_cast<double>(matrix[5])*py+matrix[13]);
        particle[1]=static_cast<float>(static_cast<double>(matrix[8])*pz+static_cast<double>(matrix[4])*py+static_cast<double>(matrix[0])*px+matrix[12]);
        const float dx=direction[0],dy=direction[1],dz=direction[2];
        direction[2]=static_cast<float>(static_cast<double>(matrix[10])*dz+static_cast<double>(matrix[2])*dx+static_cast<double>(matrix[6])*dy);
        direction[1]=static_cast<float>(static_cast<double>(matrix[9])*dz+static_cast<double>(matrix[1])*dx+static_cast<double>(matrix[5])*dy);
        direction[0]=static_cast<float>(static_cast<double>(matrix[8])*dz+static_cast<double>(matrix[4])*dy+static_cast<double>(matrix[0])*dx);
        const double scale=psmRuntimeField<float>(self,0xE4);
        const float inheritedX=static_cast<float>(scale*psmRuntimeField<float>(self,0xD4));
        const float inheritedY=static_cast<float>(scale*psmRuntimeField<float>(self,0xD8));
        const float vy=direction[1]*speed,vz=direction[2]*speed;
        particle[37]=static_cast<float>(vz+scale*psmRuntimeField<float>(self,0xDC));
        particle[35]=static_cast<float>(static_cast<double>(direction[0])*speed+inheritedX);
        particle[36]=vy+inheritedY;
    }
    else
    {
        particle[37]=static_cast<float>(rotatedZ*speed);
        particle[35]=direction[0]*speed;particle[36]=rotatedY*speed;
    }
    particle[25]=matrix[8];particle[26]=matrix[9];particle[27]=matrix[10];
    const unsigned int sections=psmRuntimeField<unsigned int>(psmRuntimeField<void*>(self,0x2C),0x220);
    const float* corners=psmRuntimeField<float*>(g_ParticleSystemManager,0x8594+4*sections);
    for(unsigned int i=0;i<2*psmRuntimeField<unsigned int>(psmRuntimeField<void*>(self,0x2C),0x220);++i)
    {
        const float* c=corners+3*i;
        const double z=static_cast<double>(c[2])*matrix[10]+static_cast<double>(matrix[2])*c[0]+static_cast<double>(c[1])*matrix[6];
        const double y=static_cast<double>(matrix[1])*c[0]+static_cast<double>(c[1])*matrix[5]+static_cast<double>(c[2])*matrix[9];
        particle[7+3*i]=static_cast<float>(static_cast<double>(c[1])*matrix[4]+static_cast<double>(matrix[0])*c[0]+static_cast<double>(matrix[8])*c[2]);
        particle[8+3*i]=static_cast<float>(y);particle[9+3*i]=static_cast<float>(z);
    }
    unsigned int index=0;
    if(psmRuntimeField<unsigned int>(self,0x28))
    {
        index=psmRuntimeField<unsigned int>(self,0x174);
        psmRuntimeField<unsigned int>(self,0x174)=(index+1)&psmRuntimeField<unsigned int>(self,0x17C);
        ++psmRuntimeField<unsigned int>(self,0x28);
    }
    else
    {psmRuntimeField<unsigned int>(self,0x174)=1;psmRuntimeField<unsigned int>(self,0x178)=0;psmRuntimeField<unsigned int>(self,0x28)=1;}
    psmRuntimeField<float**>(self,0x14)[index]=particle;
    psmRuntimeField<unsigned int>(self,0x194)=index;
    return particle;
}

// PE1017B8E0: speed threshold, recently visible gate, timed emission and moving-emitter catch-up.
void psmType2Emit(void* self,float dt)
{
    if(!psmRuntimeField<unsigned char>(self,0x140))return;
    if(dt>0.001f)
    {
        const float dz=psmRuntimeField<float>(self,0xBC)-psmRuntimeField<float>(self,0xC8);
        const float inverse=static_cast<float>(1.0/dt);
        const float vx=static_cast<float>((static_cast<double>(psmRuntimeField<float>(self,0xB4))-psmRuntimeField<float>(self,0xC0))*inverse);
        const double vy=(static_cast<double>(psmRuntimeField<float>(self,0xB8))-psmRuntimeField<float>(self,0xC4))*inverse;
        const double vz=static_cast<double>(dz)*inverse;
        if(vz*vz+vy*vy+static_cast<double>(vx)*vx<psmRuntimeField<float>(psmRuntimeField<void*>(self,0x2C),0x230))
        {psmRuntimeField<float>(self,0xF4)=psmRuntimeField<float>(self,0xF0);return;}
    }
    const unsigned int frame=static_cast<dx9rend::IRenderer*>(dx9rend::g_rendererBase)->getFrameOrdinal();
    if(psmRuntimeField<unsigned int>(self,0x120)+60u<frame)
    {psmRuntimeField<unsigned char>(self,0x157)=psmRuntimeField<unsigned char>(self,0x156);psmRuntimeField<unsigned char>(self,0x156)=0;return;}
    void* descriptor=psmRuntimeField<void*>(self,0x2C);
    if(psmRuntimeField<float>(self,0x128)<psmRuntimeField<float>(descriptor,0x94))psmRuntimeField<float>(self,0x128)=psmRuntimeField<float>(descriptor,0x94);
    if(psmRuntimeField<float>(self,0x12C)<psmRuntimeField<float>(descriptor,0x94))psmRuntimeField<float>(self,0x12C)=psmRuntimeField<float>(descriptor,0x94);
    const float interval=static_cast<float>(1.0/(static_cast<double>(psmRuntimeField<float>(descriptor,0x10))
        *psmRuntimeField<float>(self,0x128)*psmRuntimeField<float>(self,0x12C)));
    if(psmRuntimeField<unsigned char>(self,0x154))
    {psmRuntimeField<Vec3>(self,0xC0)=psmRuntimeField<Vec3>(self,0xB4);psmRuntimeField<unsigned char>(self,0x154)=0;}
    while(interval+static_cast<double>(psmRuntimeField<float>(self,0xF4))<psmRuntimeField<float>(self,0xF0))
    {
        if(psmRuntimeField<float>(self,0xE8)!=0)
        {
            const double remaining=static_cast<double>(psmRuntimeField<float>(self,0xE8))-interval;
            psmRuntimeField<float>(self,0xE8)=static_cast<float>(remaining);
            if(remaining<=0)
            {
                IParticleSystemInstance* instance=static_cast<IParticleSystemInstance*>(self);
                instance->reserved04();
                if(psmRuntimeField<unsigned char>(psmRuntimeField<void*>(self,0x2C),0x16C))instance->reserved03(false);
                return;
            }
        }
        const float next=static_cast<float>(interval+static_cast<double>(psmRuntimeField<float>(self,0xF4)));
        psmRuntimeField<float>(self,0xF4)=next;
        double fraction=0;
        if(psmRuntimeField<float>(self,0xF0)>psmRuntimeField<float>(self,0xCC))
            fraction=(static_cast<double>(next)-psmRuntimeField<float>(self,0xCC))
                /(static_cast<double>(psmRuntimeField<float>(self,0xF0))-psmRuntimeField<float>(self,0xCC));
        const Vec3 previous=psmRuntimeField<Vec3>(self,0xC0),current=psmRuntimeField<Vec3>(self,0xB4);
        const float dz=current.z-previous.z;
        const float dx=static_cast<float>((static_cast<double>(current.x)-previous.x)*fraction);
        const float dy=static_cast<float>((static_cast<double>(current.y)-previous.y)*fraction);
        psmRuntimeField<Vec3>(self,0xA4)={dx+previous.x,dy+previous.y,static_cast<float>(fraction*dz+previous.z)};
        if(psmRuntimeField<unsigned int>(self,0x28)<psmRuntimeField<unsigned int>(self,0x150))
        {
            float* particle=psmType2NewParticle(self);
            if(particle)
            {
                particle[35]+=psmRuntimeField<float>(self,0x1C);particle[36]+=psmRuntimeField<float>(self,0x20);particle[37]+=psmRuntimeField<float>(self,0x24);
                const unsigned int steps=static_cast<unsigned int>(static_cast<int>((static_cast<double>(psmRuntimeField<float>(self,0xF0))-psmRuntimeField<float>(self,0xF4))*100.0));
                if(psmType2AdvanceParticle(self,particle,psmRuntimeField<unsigned int>(self,0x28)-1,0.01f,steps))
                {
                    const float remainder=static_cast<float>(static_cast<double>(psmRuntimeField<float>(self,0xF0))-psmRuntimeField<float>(self,0xF4)-static_cast<double>(steps)*0.01f);
                    psmType2AdvanceParticle(self,particle,psmRuntimeField<unsigned int>(self,0x28)-1,remainder,1);
                }
            }
        }
    }
}
