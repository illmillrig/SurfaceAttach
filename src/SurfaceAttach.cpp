
#include "SurfaceAttach.h"

#include <maya/MGlobal.h>
#include <maya/MEulerRotation.h>


MTypeId SurfaceAttach::id(0x00121BC4);

MObject SurfaceAttach::direction;
MObject SurfaceAttach::surface;
MObject SurfaceAttach::parentInverse;
MObject SurfaceAttach::samples;
MObject SurfaceAttach::staticLength;
MObject SurfaceAttach::offset;
MObject SurfaceAttach::genus;
MObject SurfaceAttach::reverse;

MObject SurfaceAttach::inU;
MObject SurfaceAttach::inV;
MObject SurfaceAttach::inUV;

MObject SurfaceAttach::translateX;
MObject SurfaceAttach::translateY;
MObject SurfaceAttach::translateZ;
MObject SurfaceAttach::translate;

MObject SurfaceAttach::rotateX;
MObject SurfaceAttach::rotateY;
MObject SurfaceAttach::rotateZ;
MObject SurfaceAttach::rotate;

MObject SurfaceAttach::out;

SurfaceAttach::SurfaceAttach(){}

SurfaceAttach::~SurfaceAttach(){}

void* SurfaceAttach::creator() {
    return new SurfaceAttach();
}

MStatus SurfaceAttach::compute(const MPlug& plug, MDataBlock& dataBlock) {
    if (plug != out && plug != translate & plug != rotate)
        return MS::kUnknownParameter;

    const short dataDirection = dataBlock.inputValue(SurfaceAttach::direction).asShort();
    const std::int32_t dataSamples = dataBlock.inputValue(SurfaceAttach::samples).asInt();
    const short dataGenus = dataBlock.inputValue(SurfaceAttach::genus).asShort();
    const double dataOffset = dataBlock.inputValue(SurfaceAttach::offset).asDouble();
    const bool dataReverse = dataBlock.inputValue(SurfaceAttach::reverse).asBool();
    const double dataStaticLength = dataBlock.inputValue(SurfaceAttach::staticLength).asDouble();
    const MMatrix dataParentInverse = dataBlock.inputValue(SurfaceAttach::parentInverse).asMatrix();

    MFnNurbsSurface fnSurface (dataBlock.inputValue(SurfaceAttach::surface).asNurbsSurface());
    MFnDependencyNode fnDepend (plug.node());

    // Set UV Inputs
    this->inUVs(fnDepend);

    // If Percentage or Fixed Length
    if (dataGenus > 0){
        // If the amount of samples has changed, rebuild the arrays/data
        if (this->sampleCount != dataSamples) {
            this->allocate(dataSamples);
            this->sampleCount = dataSamples;
        }

        // Calculate Lengths
        if (!dataDirection)
            this->surfaceLengthsU(fnSurface, 0.5);
        else
            this->surfaceLengthsV(fnSurface, 0.5);
    }

    // Set all the existing out Plugs
    this->setOutPlugs(dataBlock, fnSurface, dataOffset, dataReverse, dataGenus, dataStaticLength,
                      dataParentInverse, dataDirection);

    return MS::kSuccess;
}

void SurfaceAttach::inUVs(MFnDependencyNode &fn) {
    MPlug inUVPlug = fn.findPlug("inUV");
    const std::uint32_t numElements = inUVPlug.numElements();

    this->uInputs.resize((size_t)(numElements));
    this->vInputs.resize((size_t)(numElements));

    for (std::uint32_t i=0; i < numElements; i++){
        MPlug uvPlug = inUVPlug.elementByLogicalIndex(i);
        this->uInputs[i] = uvPlug.child(0).asDouble();
        this->vInputs[i] = uvPlug.child(1).asDouble();
    }
}

void SurfaceAttach::allocate(const std::int32_t dataSamples) {
    this->sampler.clear();
    this->distances.clear();

    this->sampler.resize(dataSamples);
    this->distances.resize(dataSamples);

    double sample = 0.0;
    for (std::int32_t i=0; i < dataSamples; i++){
        sample = (i + 1.0) / dataSamples;
        this->sampler[i] = sample;
        this->distances[i] = 0.0;
    }
}

void SurfaceAttach::surfaceLengthsU(const MFnNurbsSurface &fnSurface, const double parmV) {
    MPoint pointA;
    fnSurface.getPointAtParam(0.0, parmV, pointA, MSpace::Space::kWorld);

    this->length = 0.0;
    MPoint pointB;

    double *s = &this->sampler[0];
    double *d = &this->distances[0];

    for (size_t i=0; i < this->sampleCount; i++){
        fnSurface.getPointAtParam(*s++, parmV, pointB, MSpace::Space::kWorld);

        // Add the measured distanced to 'length' and set the measured length in 'distances'
        this->length += pointA.distanceTo(pointB);
        *d++ = this->length;

        pointA.x = pointB.x;
        pointA.y = pointB.y;
        pointA.z = pointB.z;
    }

}

void SurfaceAttach::surfaceLengthsV(const MFnNurbsSurface &fnSurface, const double parmU) {
    MPoint pointA;
    fnSurface.getPointAtParam(parmU, 0.0, pointA, MSpace::Space::kWorld);

    this->length = 0.0;
    MPoint pointB;

    double *s = &this->sampler[0];
    double *d = &this->distances[0];

    for (size_t i=0; i < this->sampleCount; i++){
        fnSurface.getPointAtParam(parmU, *s++, pointB, MSpace::Space::kWorld);

        // Add the measured distanced to 'length' and set the measured length in 'distances'
        this->length += pointA.distanceTo(pointB);
        *d++ = this->length;

        pointA.x = pointB.x;
        pointA.y = pointB.y;
        pointA.z = pointB.z;
    }
}

void SurfaceAttach::setOutPlugs(MDataBlock dataBlock, const MFnNurbsSurface &fnSurface,
                                const double dataOffset, const bool dataReverse, const short dataGenus, const double dataStaticLength,
                                const MMatrix &dataParentInverse, const short dataDirection) {

    MTransformationMatrix tfm;
    MVector t;
    MEulerRotation r;

    MArrayDataHandle outputHandle = dataBlock.outputArrayValue(SurfaceAttach::out);
    std::int32_t count = outputHandle.elementCount();
    MDataHandle o;

    for (unsigned int k = 0; k < count; ++k) {
        outputHandle.jumpToElement(k);

        // Get Transformations
        tfm = this->matrix(fnSurface, outputHandle.elementIndex(), dataOffset, dataReverse, dataGenus,
                           dataStaticLength, dataParentInverse, dataDirection);
        t = tfm.translation(MSpace::Space::kWorld);
        r = tfm.eulerRotation();

        o = outputHandle.outputValue();
        o.child(SurfaceAttach::translate).set(t);
        o.child(SurfaceAttach::rotate).set(r.x, r.y, r.z);
    }

    // Mark Clean
    dataBlock.setClean(SurfaceAttach::translate);
    dataBlock.setClean(SurfaceAttach::rotate);
    dataBlock.setClean(SurfaceAttach::out);
}

MTransformationMatrix SurfaceAttach::matrix(const MFnNurbsSurface &fnSurface, const int plugID, const double dataOffset,
                                            const bool dataReverse, const short dataGenus, const double dataStaticLength,
                                            const MMatrix &dataParentInverse, const short dataDirection) {
    // Do all the Fancy stuff to input UV values
    double parmU = this->uInputs[plugID];
    double parmV = this->vInputs[plugID];

    // Fix U Flipping from 1.0 to 0.0
    if (uInputs[plugID] == 1.0 && parmU == 0.0)
        parmU = 1.0;
    if (vInputs[plugID] == 1.0 && parmV == 0.0)
        parmV = 1.0;

    if (dataDirection == 0)
        this->calculateUV(plugID, dataOffset, dataReverse, dataGenus, dataStaticLength, parmU);
    else
        this->calculateUV(plugID, dataOffset, dataReverse, dataGenus, dataStaticLength, parmV);

    // Calculate transformations from UV values
    const MVector normal = fnSurface.normal(parmU, parmV, MSpace::Space::kWorld);

    MPoint point;
    fnSurface.getPointAtParam(parmU, parmV, point, MSpace::Space::kWorld);

    MVector tanU, tanV;
    fnSurface.getTangents(parmU, parmV, tanU, tanV, MSpace::Space::kWorld);

    const double dubArray[4][4] = { tanU.x, tanU.y, tanU.z, 0.0,
                                    normal.x, normal.y, normal.z, 0.0,
                                    tanV.x, tanV.y, tanV.z, 0.0,
                                    point.x, point.y, point.z, 1.0 };
    const MMatrix mat (dubArray);

    return MTransformationMatrix (mat * dataParentInverse);
}

void SurfaceAttach::calculateUV(const std::int32_t plugID, const double dataOffset, const double dataReverse, const short dataGenus,
                                const double dataStaticLength, double &parm) {
    // Offset
    const double sum = parm + dataOffset;
    if (sum >= 0.0)
        parm = fmod(sum, 1.0);
    else
        parm = 1.0 - fmod(1.0 - sum, 1.0);

    // Fix Flipping from 1.0 to 0.0
    if (uInputs[plugID] == 1.0 && parm == 0.0)
        parm = 1.0;

    // Reverse
    if (dataReverse)
        parm = 1.0 - parm;

    // Percentage
    if (dataGenus > 0){
        double ratio = 1.0;

        // Fixed Length
        if (dataGenus == 2){

//            // if we want the distances between transforms to compress when the measured length is less than
//            // the static length
//            if (dataStaticLength < length)
//                ratio = dataStaticLength / length;

            ratio = dataStaticLength / length;
        }

        const double parmLength = length * (parm * ratio);
        parm = std::max(0.0, std::min(1.0, this->parmFromLength(parmLength)));
    }
}

double SurfaceAttach::parmFromLength(const double distance) {
    const size_t index = this->binSearch(distance);

    const double distA = this->distances[index];
    const double uA = this->sampler[index];

    const double distB = this->distances[index+1];
    const double uB = this->sampler[index+1];

    const double distRatio = (distance - distA) / (distB - distA);

    return uA + ((uB - uA) * distRatio);
}

size_t SurfaceAttach::binSearch(const double distance) {
    size_t a = 0;
    size_t b = this->distances.size()-1;
    size_t c = 0;
    size_t pivot;

    for (size_t i=0; i < this->distances.size(); i++){

        pivot = (a + b) / 2;

        // this is probably never true, since its comparing doubles. so this loop most likely
        // loops for the entire amount of samples :(
        if (this->distances[pivot] == distance)
            return pivot;
        else if (b - a == 1)
            return a;
        else if (this->distances[pivot] < distance)
            a = pivot;
        else
            b = pivot;
    }
    return c;
}

MStatus SurfaceAttach::initialize() {
    MFnTypedAttribute fnTypeAttr;
    MFnNumericAttribute fnNumAttr;
    MFnUnitAttribute fnUnitAttr;
    MFnCompoundAttribute fnCompoundAttr;
    MFnEnumAttribute fnEnumAttr;
    MFnMatrixAttribute fnMatAttr;

    MStatus stat;

    // Input Attributes
    direction = fnEnumAttr.create("direction", "dire", 0);
    fnEnumAttr.addField("U", 0);
    fnEnumAttr.addField("V", 1);

    surface = fnTypeAttr.create("surface", "surface", MFnData::kNurbsSurface);

    parentInverse = fnMatAttr.create("parentInverse", "ps", MFnMatrixAttribute::kDouble);
    fnMatAttr.setKeyable(true);

    samples = fnNumAttr.create("samples", "samples", MFnNumericData::kInt, 1000);
    fnNumAttr.setKeyable(true);
    fnNumAttr.setMin(1.0);

    staticLength = fnNumAttr.create("staticLength", "staticLength", MFnNumericData::kDouble, 0.0001);
    fnNumAttr.setKeyable(true);
    fnNumAttr.setMin(0.0001);

    offset = fnNumAttr.create("offset", "offset", MFnNumericData::kDouble, 0.0);
    fnNumAttr.setKeyable(true);

    genus = fnEnumAttr.create("type", "type", 0);
    fnEnumAttr.addField("Parametric", 0);

    fnEnumAttr.addField("Percentage", 1);
    fnEnumAttr.addField("FixedLength", 2);
    fnEnumAttr.setKeyable(true);

    reverse = fnNumAttr.create("reverse", "reverse", MFnNumericData::kBoolean, false);
    fnNumAttr.setKeyable(true);

    inU = fnNumAttr.create("inU", "U", MFnNumericData::kDouble, 0.5);
    fnNumAttr.setKeyable(true);

    inV = fnNumAttr.create("inV", "V", MFnNumericData::kDouble, 0.5);
    fnNumAttr.setKeyable(true);

    inUV = fnCompoundAttr.create("inUV", "inUV");
    fnCompoundAttr.setKeyable(true);
    fnCompoundAttr.setArray(true);
    fnCompoundAttr.addChild(inU);
    fnCompoundAttr.addChild(inV);
    fnCompoundAttr.setUsesArrayDataBuilder(true);

    // Output Attributes
    translateX = fnNumAttr.create("translateX", "translateX", MFnNumericData::kDouble);
    fnNumAttr.setWritable(false);
    fnNumAttr.setStorable(false);

    translateY = fnNumAttr.create("translateY", "translateY", MFnNumericData::kDouble);
    fnNumAttr.setWritable(false);
    fnNumAttr.setStorable(false);

    translateZ = fnNumAttr.create("translateZ", "translateZ", MFnNumericData::kDouble);
    fnNumAttr.setWritable(false);
    fnNumAttr.setStorable(false);

    translate = fnNumAttr.create("translate", "translate", translateX, translateY, translateZ);
    fnNumAttr.setWritable(false);
    fnNumAttr.setStorable(false);

    rotateX = fnUnitAttr.create("rotateX", "rotateX", MFnUnitAttribute::kAngle);
    fnUnitAttr.setWritable(false);
    fnUnitAttr.setStorable(false);

    rotateY = fnUnitAttr.create("rotateY", "rotateY", MFnUnitAttribute::kAngle);
    fnUnitAttr.setWritable(false);
    fnUnitAttr.setStorable(false);

    rotateZ = fnUnitAttr.create("rotateZ", "rotateZ", MFnUnitAttribute::kAngle);
    fnUnitAttr.setWritable(false);
    fnUnitAttr.setStorable(false);

    rotate = fnNumAttr.create("rotate", "rotate", rotateX, rotateY, rotateZ);
    fnNumAttr.setWritable(false);

    out = fnCompoundAttr.create("out", "out");
    fnCompoundAttr.setWritable(false);
    fnCompoundAttr.setArray(true);
    fnCompoundAttr.addChild(translate);
    fnCompoundAttr.addChild(rotate);
    fnCompoundAttr.setUsesArrayDataBuilder(true);

    // These aren't going to fail, give me a break :)
    // Add Attributes
    SurfaceAttach::addAttribute(direction);
    SurfaceAttach::addAttribute(surface);
    SurfaceAttach::addAttribute(parentInverse);
    SurfaceAttach::addAttribute(samples);
    SurfaceAttach::addAttribute(staticLength);
    SurfaceAttach::addAttribute(offset);
    SurfaceAttach::addAttribute(genus);
    SurfaceAttach::addAttribute(reverse);
    SurfaceAttach::addAttribute(inUV);
    SurfaceAttach::addAttribute(out);

    // Attribute Affects
    SurfaceAttach::attributeAffects(direction, translate);
    SurfaceAttach::attributeAffects(surface, translate);
    SurfaceAttach::attributeAffects(parentInverse, translate);
    SurfaceAttach::attributeAffects(staticLength, translate);
    SurfaceAttach::attributeAffects(samples, translate);
    SurfaceAttach::attributeAffects(offset, translate);
    SurfaceAttach::attributeAffects(genus, translate);
    SurfaceAttach::attributeAffects(reverse, translate);
    SurfaceAttach::attributeAffects(inU, translate);
    SurfaceAttach::attributeAffects(inV, translate);

    SurfaceAttach::attributeAffects(direction, rotate);
    SurfaceAttach::attributeAffects(surface, rotate);
    SurfaceAttach::attributeAffects(parentInverse, rotate);
    SurfaceAttach::attributeAffects(staticLength, rotate);
    SurfaceAttach::attributeAffects(samples, rotate);
    SurfaceAttach::attributeAffects(offset, rotate);
    SurfaceAttach::attributeAffects(genus, rotate);
    SurfaceAttach::attributeAffects(reverse, rotate);
    SurfaceAttach::attributeAffects(inU, rotate);
    SurfaceAttach::attributeAffects(inV, rotate);

    return MS::kSuccess;
}











