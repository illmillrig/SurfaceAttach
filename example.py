
# basic application example


import maya.OpenMaya as om


COUNT = 2000

fnDepend = om.MFnDependencyNode()
dgMod = om.MDGModifier()

sel = om.MSelectionList()
om.MGlobal.getActiveSelectionList(sel)

surfaceDag = om.MDagPath()
sel.getDagPath(0, surfaceDag)
surfaceDag.extendToShape()

fnDepend.setObject(surfaceDag.node())

worldOutPlug = fnDepend.findPlug("worldSpace").elementByLogicalIndex(0)

# Create SurfaceAttach
fnDepend.create("SurfaceAttach")
surfacePlug = fnDepend.findPlug("surface")

# Connect Surface
dgMod.connect(worldOutPlug, surfacePlug)

# Create Nulls
uValues = [(i, (i+1.0) / COUNT) for i in xrange(COUNT)]

outPlug = fnDepend.findPlug("out")
inPlug = fnDepend.findPlug("inUV")

for j, uVal in uValues:
    inElement = inPlug.elementByLogicalIndex(j)
    inU = inElement.child(0)
    inV = inElement.child(1)
    
    inU.setFloat(uVal)
    inV.setFloat(0.5)
    
    outElement = outPlug.elementByLogicalIndex(j)
    outT = outElement.child(0)
    outR = outElement.child(1)
    
    loc = fnDepend.create("locator")
    inT = fnDepend.findPlug("translate")
    inR = fnDepend.findPlug("rotate")

    dgMod.connect(outT, inT)
    dgMod.connect(outR, inR)
    
dgMod.doIt()   

