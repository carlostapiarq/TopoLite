//
// Created by ziqwang on 19.05.20.
//

#ifndef TOPOLITE_GUI_TOPOMANAGER_H
#define TOPOLITE_GUI_TOPOMANAGER_H

#include "IO/JsonIOReader.h"
#include "IO/JsonIOWriter.h"
#include "IO/XMLIO_backward.h"
#include "gui_Arcball_Canvas.h"
#include "gui_2D_Canvas.h"
#include "gui_PolyMeshLists.h"
#include "gui_Lines.h"


#include "CrossMesh/CrossMeshCreator.h"
#include "Structure/StrucCreator.h"

#include "nanogui/object.h"
#include <memory>
#include <Eigen/Dense>

class gui_TopoManager{
public:
    shared_ptr<IOData> iodata;
    shared_ptr<CrossMeshCreator<double>> crossMeshCreator;
    shared_ptr<StrucCreator<double>> strucCreator;

public:
    Eigen::Matrix4d init_textureMat;
    Eigen::Matrix4d last_textureMat;
    shared_ptr<InputVarList> render_update_list;

public:
    nanogui::ref<gui_Arcball_Canvas> arcball_canvas;
    nanogui::ref<gui_2D_Canvas> pattern_canvas;

public:
    gui_TopoManager(nanogui::ref<gui_Arcball_Canvas> arcball, nanogui::ref<gui_2D_Canvas> pattern){
        arcball_canvas = arcball;
        pattern_canvas = pattern;
        clear();
    }

public:

    void clear(){
        init_textureMat = Eigen::Matrix4d::Identity();
        last_textureMat = Eigen::Matrix4d::Identity();
        init_render_update_list();
    }

    void init_render_update_list(){
        render_update_list = make_shared<InputVarList>();
        render_update_list->add((bool)false, "update_cross_mesh",  "");
        render_update_list->add((bool)false, "update_augmented_vectors",  "");
        render_update_list->add((bool)false, "update_base_mesh_2D",  "");
        render_update_list->add((bool)false, "update_struc",  "");
    }

public:

    void load_from_IOData()
    {
        //0)
        crossMeshCreator = make_shared<CrossMeshCreator<double>>(iodata->varList);
        last_textureMat = init_textureMat = iodata->varList->getMatrix4d("texturedMat");

        //1) set crossMeshCreator
        if(iodata->reference_surface){
            crossMeshCreator->setReferenceSurface(iodata->reference_surface);
        }
        if(iodata->pattern_mesh){
            crossMeshCreator->setPatternMesh(iodata->pattern_mesh);
        }

        //2) create the cross mesh
        if(iodata->cross_mesh != nullptr){
            crossMeshCreator->setCrossMesh(iodata->cross_mesh);
            crossMeshCreator->updateCrossMeshBoundary(iodata->varList->getIntList("boundary_crossIDs"));
        }
        else if(iodata->reference_surface != nullptr && iodata->pattern_mesh != nullptr){
            crossMeshCreator->createCrossMeshFromRSnPattern(false, init_textureMat);
            crossMeshCreator->createAugmentedVectors();
        }

        //3) create the structure
        if(crossMeshCreator->crossMesh){
            strucCreator = make_shared<StrucCreator<double>>(iodata->varList);
            strucCreator->compute(crossMeshCreator->crossMesh);
        }
    }

    bool load_from_xmlfile(string xmlFileName)
    {
        XMLIO_backward IO;
        clear();
        iodata = make_shared<IOData>();
        if(IO.XMLReader(xmlFileName, *iodata) && iodata->varList) {
            iodata->varList->add((int)1, "layerOfBoundary",  "");
            load_from_IOData();
            return true;
        }
        return false;
    }

    bool load_from_jsonfile(string xmlFileName){
        iodata = make_shared<IOData>();
        JsonIOReader reader(xmlFileName, iodata);
        clear();
        if(reader.read()){
            load_from_IOData();
            return true;
        }
        return false;
    }

    void write_to_json(string jsonFileName)
    {
        if(crossMeshCreator){
            if(crossMeshCreator->referenceSurface){
                iodata->reference_surface = crossMeshCreator->referenceSurface;
            }

            if(crossMeshCreator->pattern2D){
                iodata->pattern_mesh = crossMeshCreator->pattern2D->getPolyMesh();
            }

            if(crossMeshCreator->crossMesh){
                iodata->cross_mesh = crossMeshCreator->crossMesh;
                iodata->varList->add(crossMeshCreator->crossMesh->getBoundaryCrossIDs(), "boundary_crossIDs", "");
            }

            iodata->varList->add(last_textureMat, "texturedMat", "");
        }
        JsonIOWriter writer(jsonFileName, iodata);
        writer.write();
    }

    void init_main_canvas(){
        if(crossMeshCreator){
            vector<shared_ptr<PolyMesh<double>>> meshLists;
            vector<nanogui::Color> colors;
            shared_ptr<CrossMesh<double>> crossMesh = crossMeshCreator->crossMesh;

            arcball_canvas->init_render_pass();
            //CrossMesh
            if(crossMesh)
            {
                colors.push_back(nanogui::Color(200, 200 ,200, 255));
                meshLists.push_back(crossMesh->getPolyMesh());
                shared_ptr<gui_PolyMeshLists<double>> polyMeshLists = make_shared<gui_PolyMeshLists<double>>(meshLists, colors, arcball_canvas->scene->render_pass);
                arcball_canvas->scene->objects.push_back(polyMeshLists);
            }

            //Augmented Vectors
            if(crossMesh)
            {
                vector<gui_LinesGroup<double>> linegroups;
                gui_LinesGroup<double> lg;
                lg.color = nanogui::Color(200, 100, 0, 255);
                for(int id = 0; id < crossMesh->size(); id++){
                    shared_ptr<Cross<double>> cross = crossMesh->cross(id);
                    for(int jd = 0; jd < cross->size(); jd++){
                        shared_ptr<OrientPoint<double>> ori = cross->ori(jd);
                        lg.lines.push_back(Line<double>(ori->point, ori->point + ori->normal * 0.03));
                    }
                }
                linegroups.push_back(lg);
                shared_ptr<gui_Lines<double>> LinesObject = make_shared<gui_Lines<double>>(linegroups, 0.01, arcball_canvas->scene->render_pass);
                LinesObject->visible = false;
                arcball_canvas->scene->objects.push_back(LinesObject);
            }

            //struc
            if(strucCreator)
            {
                meshLists.clear();
                colors.clear();
                for(auto block: strucCreator->blocks){
                    if(block && block->polyMesh){
                        if(!block->at_boundary()){
                            colors.push_back(nanogui::Color(250, 250 ,250, 255));
                        }
                        else{
                            colors.push_back(nanogui::Color(50, 50 ,50, 255));
                        }
                        meshLists.push_back(block->polyMesh);
                    }
                }
                shared_ptr<gui_PolyMeshLists<double>> polyMeshLists = make_shared<gui_PolyMeshLists<double>>(meshLists, colors, arcball_canvas->scene->render_pass);
                arcball_canvas->scene->objects.push_back(polyMeshLists);
            }

            arcball_canvas->refresh_trackball_center();
        }
    }

    void init_pattern_canvas()
    {
        vector<shared_ptr<PolyMesh<double>>> meshLists;
        vector<nanogui::Color> colors;
        shared_ptr<gui_PolyMeshLists<double>> polyMeshLists;

        pattern_canvas->init_render_pass();
        
        //baseMesh2D
        {
            meshLists.clear();
            if(crossMeshCreator->crossMesh->baseMesh2D)
            {
                shared_ptr<PolyMesh<double>> baseMesh = crossMeshCreator->crossMesh->baseMesh2D;
                meshLists.push_back(baseMesh);
            }
            
            polyMeshLists = make_shared<gui_PolyMeshLists<double>>(meshLists, colors, pattern_canvas->scene->render_pass);
            polyMeshLists->varList->add(false, "show_face", "");
            polyMeshLists->line_color = nanogui::Color(242, 133, 0, 255);
            pattern_canvas->scene->objects.push_back(polyMeshLists);
            polyMeshLists->model_mat_fixed = true;
        }

        //Pattern2D
        {
            meshLists.clear();
            if(crossMeshCreator->pattern2D)
            {
                shared_ptr<PolyMesh<double>> pattern_mesh = crossMeshCreator->pattern2D->getPolyMesh();
                meshLists.push_back(pattern_mesh);
            }
            polyMeshLists = make_shared<gui_PolyMeshLists<double>>(meshLists, colors, pattern_canvas->scene->render_pass);
            polyMeshLists->varList->add(false, "show_face", "");
            polyMeshLists->model_init_mat = init_textureMat.cast<float>();
            polyMeshLists->line_color = nanogui::Color(150, 150, 150, 255);
            pattern_canvas->scene->objects.push_back(polyMeshLists);
        }

        //SurfaceTexture
        {
            colors.push_back(nanogui::Color(255, 255 ,255, 255));
            meshLists.clear();
            if(crossMeshCreator->referenceSurface)
            {
                shared_ptr<PolyMesh<double>> textureMesh =crossMeshCreator->referenceSurface->getTextureMesh();
                meshLists.push_back(textureMesh);
            }
            polyMeshLists = make_shared<gui_PolyMeshLists<double>>(meshLists, colors, pattern_canvas->scene->render_pass);
            polyMeshLists->varList->add(false, "show_wireframe", "");
            polyMeshLists->model_mat_fixed = true;
            pattern_canvas->scene->objects.push_back(polyMeshLists);
        }
        pattern_canvas->scene->focus_item = 2;
        pattern_canvas->refresh_trackball_center();
    }
    
public:
    
    void recompute_from_textureMat(Eigen::Matrix4d textureMat){
        if(crossMeshCreator){
            crossMeshCreator->createCrossMeshFromRSnPattern(false, textureMat);
            crossMeshCreator->createAugmentedVectors();
            if(crossMeshCreator->crossMesh){
                strucCreator->compute(crossMeshCreator->crossMesh);
                set_update_list_true({"update_base_mesh_2D", "update_cross_mesh", "update_augmented_vectors", "update_struc"});
            }
        }
    }

public:

    void update_base_mesh_2D(){
        //baseMesh2D
        vector<shared_ptr<PolyMesh<double>>> meshLists;
        vector<nanogui::Color> colors;
        shared_ptr<gui_PolyMeshLists<double>> polyMeshLists;

        shared_ptr<PolyMesh<double>> baseMesh = crossMeshCreator->crossMesh->baseMesh2D;
        meshLists.push_back(baseMesh);
        ((gui_PolyMeshLists<double> *)(pattern_canvas->scene->objects[0].get()))->update_mesh(meshLists, colors);
    }

    void update_cross_mesh(){
        //CrossMesh
        shared_ptr<CrossMesh<double>> crossMesh = crossMeshCreator->crossMesh;
        vector<shared_ptr<PolyMesh<double>>> meshLists;
        vector<nanogui::Color> colors;
        colors.push_back(nanogui::Color(200, 200 ,200, 255));
        meshLists.push_back(crossMesh->getPolyMesh());
        ((gui_PolyMeshLists<double> *)(arcball_canvas->scene->objects[0].get()))->update_mesh(meshLists, colors);
    }

    void update_augmented_vectors(){
        shared_ptr<CrossMesh<double>> crossMesh = crossMeshCreator->crossMesh;
        vector<gui_LinesGroup<double>> linegroups;
        gui_LinesGroup<double> lg;
        lg.color = nanogui::Color(200, 100, 0, 255);
        for(int id = 0; id < crossMesh->size(); id++){
            shared_ptr<Cross<double>> cross = crossMesh->cross(id);
            for(int jd = 0; jd < cross->size(); jd++){
                shared_ptr<OrientPoint<double>> ori = cross->ori(jd);
                lg.lines.push_back(Line<double>(ori->point, ori->point + ori->normal * 0.03));
            }
        }
        linegroups.push_back(lg);
        ((gui_Lines<double> *)(arcball_canvas->scene->objects[1].get()))->update_line(linegroups);
    }

    void update_struc(){
        vector<shared_ptr<PolyMesh<double>>> meshLists;
        vector<nanogui::Color> colors;
        meshLists.clear();
        colors.clear();
        for(auto block: strucCreator->blocks){
            if(block && block->polyMesh){
                if(!block->at_boundary()){
                    colors.push_back(nanogui::Color(250, 250 ,250, 255));
                }
                else{
                    colors.push_back(nanogui::Color(50, 50 ,50, 255));
                }
                meshLists.push_back(block->polyMesh);
            }
        }
        ((gui_PolyMeshLists<double> *)(arcball_canvas->scene->objects[2].get()))->update_mesh(meshLists, colors);
    }


    void set_update_list_true(vector<std::string> strs){
        if(arcball_canvas && pattern_canvas && arcball_canvas->scene && pattern_canvas->scene){
            if(pattern_canvas->scene->objects.size() >= 3
            && arcball_canvas->scene->objects.size() >= 2){
                for(std::string str : strs){
                    if(str == "update_base_mesh_2D"){
                        shared_ptr<gui_RenderObject<double>> object = pattern_canvas->scene->objects[1];
                        if(object && object->visible){
                            render_update_list->add(true, "update_base_mesh_2D", "");
                        }
                    }
                    else if(str == "update_cross_mesh"){
                        shared_ptr<gui_RenderObject<double>> object = arcball_canvas->scene->objects[0];
                        if(object && object->visible){
                            render_update_list->add(true, "update_cross_mesh", "");
                        }
                    }
                    else if(str == "update_augmented_vectors"){
                        shared_ptr<gui_RenderObject<double>> object = arcball_canvas->scene->objects[1];
                        if(object && object->visible){
                            render_update_list->add(true, "update_augmented_vectors", "");
                        }
                    }
                    else if(str == "update_struc"){
                        shared_ptr<gui_RenderObject<double>> object = arcball_canvas->scene->objects[2];
                        if(object && object->visible){
                            render_update_list->add(true, "update_struc", "");
                        }
                    }
                }
            }
        }
    }
    
    void update()
    {
        Eigen::Matrix4d textureMat = pattern_canvas->get_textureMat();
        if((textureMat - last_textureMat).norm() > 1e-5) {
            recompute_from_textureMat(textureMat);
            last_textureMat = textureMat;
        }

        if(render_update_list->getBool("update_base_mesh_2D")){
            update_base_mesh_2D();
            render_update_list->add(false, "update_base_mesh_2D", "");
        }

        if(render_update_list->getBool("update_cross_mesh")){
            update_cross_mesh();
            render_update_list->add(false, "update_cross_mesh", "");
        }

        if(render_update_list->getBool("update_augmented_vectors")){
            update_augmented_vectors();
            render_update_list->add(false, "update_augmented_vectors", "");
        }

        if(render_update_list->getBool("update_struc")){
            update_struc();
            render_update_list->add(false, "update_struc", "");
        }
        
    }

private:
    Eigen::Matrix4d toEigenMatrix(double *interactMatrix){
        Eigen::Matrix4d interactMat;
        interactMat << interactMatrix[0], interactMatrix[4], interactMatrix[8], interactMatrix[12],
            interactMatrix[1], interactMatrix[5], interactMatrix[9], interactMatrix[13],
            interactMatrix[2], interactMatrix[6], interactMatrix[10], interactMatrix[14],
            interactMatrix[3], interactMatrix[7], interactMatrix[11], interactMatrix[15];
        return interactMat;
    }
};


#endif //TOPOLITE_GUI_TOPOMANAGER_H
