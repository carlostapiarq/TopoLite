///////////////////////////////////////////////////////////////
//
// Polygon.cpp
//
//   3D Polygon (vertices may or may not be co-planar)
//
// by Peng SONG  ( songpenghit@gmail.com )
// 
// 12/Jan/2018
//
//
///////////////////////////////////////////////////////////////

#include "Utility/GeometricPrimitives.h"
#include "Utility/HelpDefine.h"
#include "Polygon.h"

//**************************************************************************************//
//                                    Initialization
//**************************************************************************************//

template <typename Scalar>
_Polygon<Scalar>::_Polygon()
{
	normal_ = center_ = Vector3(0, 0, 0);
	dist_ = polyType_ = 0;
	compute_data_dirty = false;
}

template <typename Scalar>
_Polygon<Scalar>::~_Polygon()
{
    clear();
}

template <typename Scalar>
_Polygon<Scalar>::_Polygon(const _Polygon<Scalar> &poly)
{

    vers_.clear();
    for(pVertex vertex: poly.vers_){
        vers_.push_back(make_shared<VPoint<Scalar>>(*vertex));
    }

    texs_.clear();
    for(pVTex tex: poly.texs_){
        texs_.push_back(make_shared<VTex<Scalar>>(*tex));
    }

    update();

    this->polyType = poly.polyType;
    this->dist = poly.dist;
}

//**************************************************************************************//
//                                    modify operation
//**************************************************************************************//

//1) will affect computed data
template <typename Scalar>
void _Polygon<Scalar>::setVertices(vector<Vector3> _vers)
{
    dirty();
    
    vers_.clear();
    for (int i = 0; i < _vers.size(); i++)
    {
        pVertex vertex = make_shared<VPoint<Scalar>>(_vers[i]);
        vers_.push_back(vertex);
    }
}

template <typename Scalar>
size_t _Polygon<Scalar>::push_back(Vector3 pt)
{
    dirty();

    pVertex vertex = make_shared<VPoint<Scalar>>(pt);
    vers_.push_back(vertex);
    return vers_.size();
}

template <typename Scalar>
size_t _Polygon<Scalar>::push_back(Vector3 pt, Vector2 tex)
{
    dirty();

    pVertex vertex = make_shared<VPoint<Scalar>>(pt);
    vers_.push_back(vertex);

    pVTex ptex = make_shared<VTex<Scalar>>(tex);
    texs_.push_back(ptex);

    return vers_.size();
}

//2) will not affect computed data

template <typename Scalar>
void _Polygon<Scalar>::reverseVertices()
{
    // Reverse vertices
    vector<pVertex> newVers;
    for (int i = (int)(vers_.size()) - 1; i >= 0; i--)
    {
        pVertex vertex = make_shared<VPoint<Scalar>>(*vers_[i]);
        newVers.push_back(vertex);
    }

    vers_ = newVers;

    // Reverse normal (no need recompute the normal)
    normal_ = -normal_;
}

template <typename Scalar>
void _Polygon<Scalar>::translatePolygon(Vector3 transVec)
{
    for (int i = 0; i < vers_.size(); i++)
    {
        vers_[i]->pos += transVec;
    }

    //no need to recompute normal and center
    center_ += transVec;
}


/***********************************************
 *                                             *
 *             read only      operation        *
 *                                             *
 ***********************************************/

template <typename Scalar>
bool _Polygon<Scalar>::checkEquality(const _Polygon &poly) const
{
	if (this->size() != poly.size())
		return false;

	const _Polygon<Scalar> &A = *this;
	const _Polygon<Scalar> &B = poly;

	int id;
    for(id = 0; id < A.size(); id++)
    {
        if((A.vers_[id]->pos - B.vers_[0]->pos).norm() < FLOAT_ERROR_SMALL){
            break;
        }
    }
    if(id == A.size()) return false;

    for(int jd = 0; jd < A.size(); jd++)
    {
        int Aij = (id + jd) % A.size();
        if((A.vers_[Aij]->pos - B.vers_[jd]->pos).norm() > FLOAT_ERROR_SMALL){
            return false;
        }
    }

    return true;
}

template <typename Scalar>
void _Polygon<Scalar>::print() const
{
	printf("verNum: %lu \n", vers_.size());
	for (int i = 0; i < vers_.size(); i++)
	{
		printf("(%6.3f, %6.3f, %6.3f) \n", vers_[i]->pos.x(), vers_[i]->pos.y(), vers_[i]->pos.z());
	}
	printf("\n");
}

template <typename Scalar>
Matrix<Scalar, 3, 1> _Polygon<Scalar>::computeCenter() const
{
    Vector3d _center = Vector3d(0, 0, 0);
	for (int i= 0; i < vers_.size(); i++)
	{
        _center += vers_[i]->pos;
	}
    _center = _center / vers_.size();
	return _center;
}

template <typename Scalar>
Matrix<Scalar, 3, 1> _Polygon<Scalar>::computeNormal() const
{
	Vector3 _center = computeCenter();
	Vector3 tempNor(0, 0, 0);
	for(int id = 0; id < (int)(vers_.size()) - 1; id++)
	{
	    tempNor += (vers_[id]->pos - _center).cross(vers_[id + 1]->pos - _center);
	}

	if(tempNor.norm() < FLOAT_ERROR_LARGE)
	    return Vector3(0, 0, 0);

	//normal already normalized
	Vector3 _normal = computeFitedPlaneNormal();
	if(_normal.dot(tempNor) < 0) _normal *= -1;
	return _normal;
}

//see https://www.ilikebigbits.com/2015_03_04_plane_from_points.html
template <typename Scalar>
Matrix<Scalar, 3, 1> _Polygon<Scalar>::computeFitedPlaneNormal() const
{
	if(vers_.size() < 3)
	{
		return Vector3(0, 0, 0);
	}

	Vector3 _center = computeCenter();

	double xx, xy, xz, yy, yz, zz;
	xx = xy = xz = yy = yz = zz = 0;
	for(pVertex ver: vers_)
	{
		Vector3 r = ver->pos - _center;
		xx += r.x() * r.x();
		xy += r.x() * r.y();
		xz += r.x() * r.z();
		yy += r.y() * r.y();
		yz += r.y() * r.z();
		zz += r.z() * r.z();
	}

	double det_x = yy*zz - yz*yz;
	double det_y = xx*zz - xz*xz;
	double det_z = xx*yy - xy*xy;

	double maxDet = std::max(det_x, std::max(det_y, det_z));
	if(maxDet <= 0){
		return Vector3(0, 0, 0);
	}

	Vector3d _normal;
	if(maxDet == det_x)
	{
        _normal = Vector3(det_x, xz*yz - xy*zz, xy*yz - xz*yy);
	}
	else if(maxDet == det_y)
	{
        _normal = Vector3(xz*yz - xy*zz, det_y, xy*xz - yz*xx);
	}
	else {
        _normal = Vector3(xy*yz - xz*yy, xy*xz - yz*xx, det_z);
	}

	return _normal.normalized();
}

template <typename Scalar>
Scalar _Polygon<Scalar>::computeArea() const
{
    Vector3 _normal = computeNormal();

	Scalar signedArea = 0;
	for (int i = 0; i < vers_.size(); i++)
	{
        Vector3 currVer = vers_[i]->pos;
        Vector3 nextVer = vers_[(i + 1) % vers_.size()]->pos;
		signedArea += 0.5 * (_normal.dot(currVer.cross(nextVer)));
	}

	return std::abs( signedArea );
}

template <typename Scalar>
Scalar _Polygon<Scalar>::computeAverageEdge() const
{
	Scalar avgEdgeLen = 0;

	for (int i = 0; i < vers_.size(); i++)
	{
		Vector3 currVer = vers_[i]->pos;
		Vector3 nextVer = vers_[(i + 1) % vers_.size()]->pos;
        avgEdgeLen += (nextVer - currVer).norm();
	}

	avgEdgeLen /= vers_.size();

	return avgEdgeLen;
}

template <typename Scalar>
Scalar _Polygon<Scalar>::computeMaxRadius() const
{
    Scalar MaxRadius = 0;

    Vector3 origin = computeCenter();
    for (int i = 0; i < vers_.size(); i++)
    {
        MaxRadius = std::max((vers_[i]->pos - origin).norm(), MaxRadius);
    }
    return MaxRadius;
}

template <typename Scalar>
void _Polygon<Scalar>::computeFrame(Vector3 &x_axis, Vector3 &y_axis, Vector3 &origin) const
{
	Vector3 _normal = computeNormal();
    origin = computeCenter();

	x_axis = _normal.cross(Vector3(1, 0, 0));
	if(x_axis.norm() < FLOAT_ERROR_LARGE)
	    x_axis = _normal.cross(Vector3(0, 1, 0));
	x_axis.normalize();

	y_axis = _normal.cross(x_axis);
	y_axis.normalize();
}

template <typename Scalar>
void _Polygon<Scalar>::convertToTriangles(vector<pTriangle> &tris) const
{
    if(vers_.size() < 3)
        return;

    Vector3 _center = computeCenter();
    for (int i = 0; i < vers_.size(); i++)
	{
		pTriangle tri = make_shared<Triangle<Scalar>>();
		tri->v[0] = _center;
		tri->v[1] = vers_[i]->pos;
		tri->v[2] = vers_[(i + 1)%vers_.size()]->pos;
		tris.push_back(tri);
	}

    return;
}

template <typename Scalar>
int _Polygon<Scalar>::getPtVerID(_Polygon<Scalar>::Vector3 point) const
{
	for (int i = 0; i < vers_.size(); i++)
	{
		Scalar dist = (point - vers_[i]->pos).norm();

		// Note: this threshold depends on the scale of elements
		if (dist < FLOAT_ERROR_LARGE)
		{
			return i;
		}
	}

	return ELEMENT_OUT_LIST;
}
