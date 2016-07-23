#pragma once

#include <vector> 
#include <array> 

#include <maya/MTypeId.h> 
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MStatus.h>
#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MTransformationMatrix.h>


//-------------------------------------------------------------------

class SurfaceAttach : public MPxNode {

public:
	SurfaceAttach();
	virtual ~SurfaceAttach(); 
	virtual MStatus	compute(const MPlug& plug, MDataBlock& dataBlock) override;
	static void* creator();
	static MStatus initialize();

private:
	void allocate(const int dataSamples);
	void inUVs(MFnDependencyNode &fn);
	size_t binSearch(const double distanceU);
	double uParmFromLength(const double distanceU);
	void surfaceLengths(const MFnNurbsSurface &fnSurface, const double parmV);

	void setOutPlugs(MDataBlock dataBlock, const MFnDependencyNode &fn, const MFnNurbsSurface &fnSurface,
					 const double &dataOffset, const bool &dataReverse, const short &dataGenus,
					 const double &dataStaticLength, const MMatrix &dataParentInverse);

	MTransformationMatrix matrix(const MFnNurbsSurface &fnSurface, const int plugID, const double &dataOffset,
								 const bool &dataReverse, const short &dataGenus,
								 const double &dataStaticLength, const MMatrix &dataParentInverse);

	void calculateUV(const int plugID, const double &dataOffset, const double &dataReverse,
					 const short &dataGenus, const double &dataStaticLength, double &parmU, double &parmV);

public:
	static MTypeId id;

	// Input Attribute Handles
	static MObject surface;	
	static MObject samples;
	static MObject staticLength;
	static MObject offset;
	static MObject genus;
	static MObject reverse;
	static MObject inU;
	static MObject inV;
	static MObject inUV;
	static MObject parentInverse;

	// output Attribute Handles
	static MObject translateX;		
	static MObject translateY;		
	static MObject translateZ;		
	static MObject translate;		
	static MObject rotateX;		
	static MObject rotateY;		
	static MObject rotateZ;		
	static MObject rotate;		
	static MObject out;		

private:
	// Sampling
	double length;
	int sampleCount;
	std::vector <double> sampler;
	std::vector <MPoint> samplePoints;
	std::vector <std::array<double, 2>> distances;
	std::vector <std::array<double, 2>> uvInputs;
};

