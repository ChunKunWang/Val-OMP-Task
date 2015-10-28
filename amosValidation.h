/*Chun-Kun Wang (amos@cs.unc.edu)*/

#ifndef amos_validation_INCLUDED
#define amos_validation_INCLUDED

#include "rose.h"

//Variable classification support
#include "DefUseAnalysis.h"

//OpenMP attribute for OpenMP 3.0
#include "OmpAttribute.h"

//Array Annotation headers
#include <CPPAstInterface.h>
#include <ArrayAnnot.h>
#include <ArrayRewrite.h>

//Dependence graph headers
#include <AstInterface_ROSE.h>
#include <LoopTransformInterface.h>
#include <AnnotCollect.h>
#include <OperatorAnnotation.h>
#include <LoopTreeDepComp.h>

//Standard C++ header
#include <vector>
#include <string>
#include <map>

namespace AmosValidation
{
	extern DFAnalysis *defuse;
	extern LivenessAnalysis *liv;
	extern bool enable_debug;
	extern bool enable_advice;
	extern bool enable_dump;
	extern bool enable_dumpall;
	extern bool hasTask;

	void command_processing( std::vector<std::string> &argvList );
	bool parse_OmpTask( std::vector<std::string> &argvList );
	bool transform_Task2Loop( std::vector<std::string> &argvList );
	bool amos_filter( std::vector<std::string> &argvList );
	bool amos_clear( std::vector<std::string> &argvList );

	// Return the loop invariant of a canonical loop, return NULL otherwise
	SgInitializedName* getLoopInvariant( SgNode *loop );
	bool TaskValidation( SgNode *loop );
	void TaskAutoScoping( SgNode *sg_node, OmpSupport::OmpAttribute *attribute );
	void add_synchronization( SgNode *sg_node );

	void GetLiveVariables( SgNode *loop, std::vector<SgInitializedName*> &liveIns, 
			std::vector<SgInitializedName*> &liveOuts, bool reCompute/*=false*/ );

	void CollectDefVariables( SgNode *loop, std::vector<SgInitializedName*> &resultVars );
	void CollectVisibleVariables( SgNode *loop, std::vector<SgInitializedName*> &resultVars, bool scalarOnly/*=false*/ );

	bool initialize_analysis( SgProject *project=NULL, bool debug=false );
	void release_analysis();

	long usecs( void );

} // end namespace

#endif // amos_validation_INCLUDED

