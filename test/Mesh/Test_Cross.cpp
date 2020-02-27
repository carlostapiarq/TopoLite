//
// Created by ziqwang on 2020-02-16.
//

#include <catch2/catch.hpp>
#include "Mesh/Cross.h"
#include "IO/InputVar.h"

TEST_CASE("Cross")
{
    shared_ptr<InputVarList> varList;
    varList = make_shared<InputVarList>();
    InitVarLite(varList.get());

    SECTION("Hexagon"){
        Cross<double> cross(varList);


        double radius = 1.0;
        int N = 6;
        for(int id = 0; id < N; id++){
            Vector3d pt(std::cos(2 * M_PI / N * id), std::sin(2 * M_PI / N * id), 0);
            cross.push_back(pt);
        }


        //initTiltNormal
        cross.initTiltNormals();
        for(int id = 0; id < N; id++){
            Vector3d mid = (cross.pos(id) + cross.pos(id + 1)) / 2;
            REQUIRE(Approx(mid.cross(cross.ori(id)->normal).norm()).margin(1e-10) == 0.0);
        }

        //updateTiltNormalsRoot
        cross.updateTiltNormalsRoot(-30);
        for(int id = 0; id < N; id++){
            REQUIRE(Approx(cross.ori(id)->rotation_angle).margin(1e-10) == 30.0);
            REQUIRE(cross.ori(id)->tiltSign == (id % 2 == 0?1 :-1));
        }
        REQUIRE(Approx((cross.ori(0)->normal - Vector3d(3.0/4, sqrt(3)/4, -1.0 / 2)).norm()).margin(1e-10) == 0.0);

        //updateTiltNormals
        cross.updateTiltNormals(30);
    }

    SECTION("Four Quad"){
        vector<shared_ptr<Cross<double>>> crossLists;

        int dXY[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        int verIDs[4][4] = {{0, 1, 2, 3},
                            {1, 4, 5, 2},
                            {2, 5, 6, 7},
                            {3, 2, 7, 8}};

        for(int id = 0; id < 4; id++)
        {
            shared_ptr<Cross<double>> cross = make_shared<Cross<double>>(Cross<double>(varList));
            cross->push_back(Vector3d(0 + dXY[id][0], 0 + dXY[id][1], 0));
            cross->push_back(Vector3d(1 + dXY[id][0], 0 + dXY[id][1], 0));
            cross->push_back(Vector3d(1 + dXY[id][0], 1 + dXY[id][1], 0));
            cross->push_back(Vector3d(0 + dXY[id][0], 1 + dXY[id][1], 0));

            cross->vers[0]->verID = verIDs[id][0];
            cross->vers[1]->verID = verIDs[id][1];
            cross->vers[2]->verID = verIDs[id][2];
            cross->vers[3]->verID = verIDs[id][3];

            crossLists.push_back(cross);
            cross->crossID = id;
            cross->neighbors.resize(4);
        }
        crossLists[0]->neighbors[1] = crossLists[1];
        crossLists[0]->neighbors[2] = crossLists[3];
        crossLists[1]->neighbors[3] = crossLists[0];
        crossLists[1]->neighbors[2] = crossLists[2];
        crossLists[2]->neighbors[0] = crossLists[1];
        crossLists[2]->neighbors[3] = crossLists[3];
        crossLists[3]->neighbors[1] = crossLists[2];
        crossLists[3]->neighbors[0] = crossLists[0];

        SECTION("getEdgeIDOfGivenCross")
        {
            REQUIRE(crossLists[0]->getEdgeIDOfGivenCross(crossLists[2].get()) == NONE_ELEMENT);
            REQUIRE(crossLists[0]->getEdgeIDOfGivenCross(crossLists[1].get()) == 1);
            REQUIRE(crossLists[0]->getEdgeIDOfGivenCross(crossLists[3].get()) == 2);
            REQUIRE(crossLists[1]->getEdgeIDOfGivenCross(crossLists[0].get()) == 3);
            REQUIRE(crossLists[1]->getEdgeIDOfGivenCross(crossLists[2].get()) == 2);
            REQUIRE(crossLists[2]->getEdgeIDOfGivenCross(crossLists[1].get()) == 0);
            REQUIRE(crossLists[2]->getEdgeIDOfGivenCross(crossLists[3].get()) == 3);
            REQUIRE(crossLists[3]->getEdgeIDOfGivenCross(crossLists[2].get()) == 1);
            REQUIRE(crossLists[3]->getEdgeIDOfGivenCross(crossLists[0].get()) == 0);
        }

        SECTION("getEdgeIDOfGivenVertexID"){
            REQUIRE(crossLists[1]->getEdgeIDOfGivenVertexID(4) == 1);
        }

        SECTION("getPrevEdgeID"){
            REQUIRE(crossLists[1]->getPrevEdgeID(0) == 3);
        }

        SECTION("getCrossIDSharedWithCross"){
            vector<int> shared_crossIDs;
            REQUIRE(crossLists[0]->getCrossIDsSharedWithCross(crossLists[1].get(), shared_crossIDs) == NONE_ELEMENT);
            REQUIRE(crossLists[0]->getCrossIDsSharedWithCross(crossLists[2].get(), shared_crossIDs) == 2);
            REQUIRE(shared_crossIDs[0] == 1);
            REQUIRE(shared_crossIDs[1] == 3);
        }

        SECTION("getEdgeIDSharedWithCross"){
            REQUIRE(crossLists[0]->getEdgeIDSharedWithCross(crossLists[1].get()) == 1);
            REQUIRE(crossLists[0]->getEdgeIDSharedWithCross(crossLists[2].get()) == NONE_ELEMENT);
        }

        crossLists[0]->atBoundary = true;
        crossLists[1]->atBoundary = true;
        crossLists[2]->atBoundary = true;
        crossLists[3]->atBoundary = false;
        SECTION("checkNeighborAtBoundary"){
            REQUIRE(crossLists[0]->checkNeighborAtBoundary(0) == true);
            REQUIRE(crossLists[0]->checkNeighborAtBoundary(1) == true);
            REQUIRE(crossLists[0]->checkNeighborAtBoundary(2) == false);
        }
    }
}