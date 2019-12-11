//
// Created by ziqwang on 2019-10-28.
//
#include "CSharpExtern.h"
#include <string>

#include "CrossMesh/PatternCreator.h"
#include "Utility/HelpFunc.h"
#include "Structure/Part.h"
#include "IO/XMLIO.h"
#include "Structure/StrucCreator.h"

/*
 * GLOBAL VARIABLES
 */

void getInteractMatrix(XMLData* data, double *interactMat)
{
    LoadIdentityMatrix(interactMat);

    double scaleMat[16];
    ScaleMatrix(data->interact_delta.scale, data->interact_delta.scale, data->interact_delta.scale, scaleMat);
    MultiplyMatrix(scaleMat, interactMat, interactMat);

    double transMat[16];
    TranslateMatrix(data->interact_delta.x, data->interact_delta.y, 0, transMat);
    MultiplyMatrix(transMat, interactMat, interactMat);

    double rotMat[16];
    RotateMatrix(data->interact_delta.angle, 0, 0, 1, rotMat);
    MultiplyMatrix(rotMat, interactMat, interactMat);

    MultiplyMatrix(interactMat, data->interactMatrix, interactMat);
}


//interface implementation


//IO
XMLData* readXML(const char *xmlstr)
{
    XMLData *data = new XMLData();
    XMLIO reader;
    reader.XMLReader(xmlstr, *data);

    data->interact_delta.scale = 1;

    return data;
}

XMLData* initStructure(){
    XMLData *data = new XMLData();

    data->varList = make_shared<InputVarList>();
    InitVarLite(data->varList.get());

    data->strucCreator = make_shared<StrucCreator>(data->varList);

    LoadIdentityMatrix(data->interactMatrix);

    data->interact_delta.scale = 1;

    return data;
}

PolyMeshRhino *initPartMeshPtr(int partID, XMLData *data){
    PolyMeshRhino *mesh = NULL;
    if(data && data->strucCreator && data->strucCreator->struc){
        if(0 <= partID && partID < data->strucCreator->struc->partList.size())
        {
            shared_ptr<Part> part = data->strucCreator->struc->partList[partID];
            if(part && part->polyMesh){
                MeshConverter converter(data->varList);
                mesh = new PolyMeshRhino();
                converter.Convert2EigenMesh(part->polyMesh.get(), mesh);
                return mesh;
            }
        }
    }
    return NULL;
}

PolyLineRhino *initCrossMeshPtr(XMLData *data){
    PolyLineRhino *lines = NULL;
    if(data
        && data->strucCreator
        && data->strucCreator->crossMeshCreator
        && data->strucCreator->crossMeshCreator->crossMesh)
    {
        shared_ptr<PolyMesh> crossMesh = data->strucCreator->crossMeshCreator->crossMesh->getPolyMesh();
        MeshConverter converter(data->varList);
        lines = new PolyLineRhino();
        converter.Convert2PolyLines(crossMesh.get(), lines);
        return lines;
    }

    return NULL;
}

PolyLineRhino *initPatternMeshPtr(XMLData *data){
    PolyLineRhino *lines = NULL;
    if(data
       && data->strucCreator
       && data->strucCreator->crossMeshCreator
       && data->strucCreator->crossMeshCreator->pattern2D)
    {
        shared_ptr<PolyMesh> pattern2DMesh = data->strucCreator->crossMeshCreator->pattern2D->getPolyMesh();
        MeshConverter converter(data->varList);
        double tmpMat[16];
        getInteractMatrix(data, tmpMat);
        lines = new PolyLineRhino();
        converter.Convert2PolyLines(pattern2DMesh.get(), lines, tmpMat);
        return lines;
    }

    return NULL;
}

PolyLineRhino *initBaseMesh2DPtr(XMLData *data){
    PolyLineRhino *lines = NULL;
    if(data
       && data->strucCreator
       && data->strucCreator->crossMeshCreator
       && data->strucCreator->crossMeshCreator->crossMesh)
    {
        shared_ptr<PolyMesh> baseMesh2D = data->strucCreator->crossMeshCreator->crossMesh->baseMesh2D;
        if(baseMesh2D){
            MeshConverter converter(data->varList);
            lines = new PolyLineRhino();
            double tmpMat[16];
            getInteractMatrix(data, tmpMat);
            converter.Convert2PolyLines(baseMesh2D.get(), lines, tmpMat);
            return lines;
        }
    }

    return NULL;
}

PolyLineRhino *initTextureMeshPtr(XMLData *data){
    PolyLineRhino *lines = NULL;
    if(data
       && data->strucCreator
       && data->strucCreator->crossMeshCreator
       && data->strucCreator->crossMeshCreator->referenceSurface)
    {
        shared_ptr<CrossMeshCreator> crossMeshCreator = data->strucCreator->crossMeshCreator;
        shared_ptr<PolyMesh> referenceMesh = crossMeshCreator->referenceSurface;
        if(referenceMesh->texturedModel) {
            shared_ptr<PolyMesh> textureMesh = referenceMesh->getTextureMesh();
            MeshConverter converter(data->varList);
            lines = new PolyLineRhino();
            double texToWinMatrix[16] = {2,0,0,0, 0,2,0,0, 0,0,1,0, -1,-1,0,1};
            double tmpMat[16];
            MultiplyMatrix(texToWinMatrix, crossMeshCreator->textureNormalizeMat, tmpMat);
            converter.Convert2PolyLines(textureMesh.get(), lines, tmpMat);
            return lines;
        }
    }
    return NULL;
}

int deleteStructure(XMLData* data){
    if(data){
        delete data;
        return 1;
    }
    else{
        return 0;
    }
}

int deletePolyMeshRhino(PolyMeshRhino *mesh){
    if(mesh){
        delete mesh;
        return 1;
    }
    else{
        return 0;
    }
}

int deletePolyLineRhino(PolyLineRhino *lines){
    if(lines){
        delete lines;
        return 1;
    }
    else{
        return 0;
    }
}



//Create Geometry
void refresh(XMLData* data)
{
    if (data && data->strucCreator && data->strucCreator->crossMeshCreator) {
        double tmpMat[16];
        getInteractMatrix(data, tmpMat);
        if (data->strucCreator->crossMeshCreator->aabbTree && data->strucCreator->crossMeshCreator->quadTree)
        {
            data->strucCreator->CreateStructure(true, true, tmpMat, false);
        }
        else{
            data->strucCreator->CreateStructure(true, false, tmpMat, false);
        }
    }
    return;
}

void preview(XMLData* data)
{
    if (data && data->strucCreator && data->strucCreator->crossMeshCreator) {
        double tmpMat[16];
        getInteractMatrix(data, tmpMat);
        if (data->strucCreator->crossMeshCreator->aabbTree && data->strucCreator->crossMeshCreator->quadTree) {
            data->strucCreator->CreateStructure(true, true, tmpMat, true);
        } else {
            data->strucCreator->CreateStructure(true, false, tmpMat, true);
        }
    }
    return;
}



//Get Info
int partNumber(XMLData* data){
    if(data && data->strucCreator && data->strucCreator->struc)
        return data->strucCreator->struc->partList.size();
    else
        return 0;
}

int isNull(void *mesh){
	if (mesh) return 0;
	else return 1;
}

int getNVertices(PolyMeshRhino *mesh){
    if(mesh){
        return mesh->vertices.size();
    }
    return 0;
}

int getNFaces(PolyMeshRhino *mesh){
    if(mesh){
        return mesh->faces.size();
    }
    return 0;
}

int getNPolyLines(PolyLineRhino *polylines){
    if(polylines){
        return polylines->data.size();
    }
    return 0;
}

int getNPolyLineI(PolyLineRhino *polylines, int lID){
    if(polylines && lID >= 0 && lID < getNPolyLines(polylines)){
        return polylines->data[lID].size();
    }
    return 0;
}

int getNVertexGroup(PolyMeshRhino *mesh){
    if(mesh)
    {
        return mesh->verticesGroups.size();
    }
    return 0;
}

int getNFaceGroup(PolyMeshRhino *mesh){
    if(mesh)
    {
        return mesh->facesGroups.size();
    }
    return 0;
}

int getNVertexGroupI(PolyMeshRhino *mesh, int vgID){
    if(mesh && vgID >= 0 && vgID < mesh->verticesGroups.size()){
        return mesh->verticesGroups[vgID].size();
    }
    return 0;
}

int getNFaceGroupI(PolyMeshRhino *mesh, int vfID){
    if(mesh && vfID >= 0 && vfID < mesh->facesGroups.size()){
        return mesh->facesGroups[vfID].size();
    }
    return 0;
}

int isBoundary(int partID, XMLData* data)
{
    if(data && data->strucCreator && data->strucCreator->struc)
    {
        if(0 <= partID && partID < data->strucCreator->struc->partList.size()) {
            return data->strucCreator->struc->partList[partID]->atBoundary == true ? 1 : 0;
        }
    }
    return 0;
}

float ComputeGroundHeight(XMLData* data)
{
    if(data && data->strucCreator && data->strucCreator->struc)
    {
        return data->strucCreator->struc->ComputeLowestY();
    }

    return 0;
}


//Copy Data
void copyVertexI(PolyMeshRhino *mesh, int vID, float *point){
    if(mesh && vID >= 0 && vID < mesh->vertices.size()){
        point[0] = mesh->vertices[vID][0];
        point[1] = mesh->vertices[vID][1];
        point[2] = mesh->vertices[vID][2];
    }
}

void copyFaceI(PolyMeshRhino *mesh, int fID, int *face){
    if(mesh && fID >= 0 && fID < mesh->faces.size()){
        face[0] = mesh->faces[fID][0];
        face[1] = mesh->faces[fID][1];
        face[2] = mesh->faces[fID][2];
    }
}

void copyPolyLineI(PolyLineRhino *polylines, int lID, float *points)
{
    if(polylines && lID >= 0 && lID < getNPolyLines(polylines))
    {
        for(int id = 0; id < polylines->data[lID].size(); id++)
        {
            Vector3f pt = ((polylines->data[lID])[id]);
            points[id * 3 + 0] = pt[0];
            points[id * 3 + 1] = pt[1];
            points[id * 3 + 2] = pt[2];
        }
    }
    return;
}

void copyVertexGroupI(PolyMeshRhino *mesh, int vgID, int* vg){

    //vg should be not null
    if(mesh && vgID >= 0 && vgID < mesh->verticesGroups.size() && vg)
    {
        for(int id = 0; id < mesh->verticesGroups[vgID].size(); id++){
            vg[id] = mesh->verticesGroups[vgID][id];
        }
    }
    return;
}

void copyFaceGroupI(PolyMeshRhino *mesh, int fgID, int* fg){

    //vg should be not null
    if(mesh && fgID >= 0 && fgID < mesh->facesGroups.size() && fg)
    {
        for(int id = 0; id < mesh->facesGroups[fgID].size(); id++){
            fg[id] = mesh->facesGroups[fgID][id];
        }
    }
    return;
}


//Set Parameter
void setParaDouble(const char *name, double value, XMLData* data){
    if(data && data->varList)
        data->varList->set(name,  (float)value);
    return;
}

void setPatternAngle(double angle, XMLData *data){
    if(data)
    {
        data->interact_delta.angle = angle;
    }
}

void setPatternXY(double x, double y, XMLData *data){
    if(data)
    {
        data->interact_delta.x = x;
        data->interact_delta.y = y;
    }
}

void setPatternScale(double s, XMLData *data){
    if(data)
    {
        data->interact_delta.scale = s;
    }
}