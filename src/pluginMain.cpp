
#include "SurfaceAttach.h"
#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj ) { 
	MStatus   status;
	MFnPlugin plugin( obj, "", "2016", "Any");

	status = plugin.registerNode( "SurfaceAttach", SurfaceAttach::id, SurfaceAttach::creator,
								  SurfaceAttach::initialize );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj) {
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( SurfaceAttach::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
