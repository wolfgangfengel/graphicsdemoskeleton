static void drawMesh(unsigned int mesh,float x,float y,float z,float scale,float angle,float mirror,float lit)
{
    constants.translateScale[0]=x; constants.translateScale[1]=y;
    constants.translateScale[2]=z; constants.translateScale[3]=scale;
    constants.rotateMirrorLight[0]=saCos(angle); constants.rotateMirrorLight[1]=saSin(angle);
    constants.rotateMirrorLight[2]=mirror; constants.rotateMirrorLight[3]=lit;
    vkCmdPushConstants(vkCommands,vkGraphicsLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(constants),&constants);
    vkCmdDraw(vkCommands,saMeshes[mesh].count,1,saMeshes[mesh].first,0);
}

static void drawModels(unsigned int tick,float mirror)
{
    int x,y;
    saSeed=9;
    for (y=-5;y<=5;++y) for (x=-5;x<=5;++x) {
        unsigned int shape=saRandom()%SUPERSHAPE_COUNT;
        float angle=(saRandom()%360)*(SA_PI/180);
        drawMesh(shape,(float)(x*9),(float)(y*9),0,sSuperShapeParams[shape][14],angle,mirror,1);
    }
    for (x=-2;x<=2;++x) {
        float offset=(x*4500+(int)(tick%4500))*0.01f;
        drawMesh(SUPERSHAPE_COUNT-1,offset,-4,2,1,0,mirror,1);
        drawMesh(SUPERSHAPE_COUNT-1,-4,offset,4,1,SA_PI/2,mirror,1);
    }
}

