/*Chun-Kun Wang (amos@cs.unc.edu)*/

#include "rose.h"
#include "amosValidation.h"

#include <iostream>
#include <fstream>
#include <map>
#include <iterator> //ostream_iterator
#include <time.h>

using namespace std;

//AST construction gererator
using namespace SageBuilder;
using namespace SageInterface;

//OpenMP 3.0 supportE
using namespace OmpSupport;

namespace AmosValidation
{
	bool enable_debug = false;
	bool enable_advice = false;
	bool enable_dump = false;
	bool enable_dumpall = false;
	bool hasTask = false;
	DFAnalysis *defuse = NULL;
	LivenessAnalysis *liv = NULL;

	void command_processing( std::vector<std::string> &argvList )
	{
		// project for openmp version with directive parseing
		argvList.push_back( "-rose:openmp:ast_only" );

		if( CommandlineProcessing::isOption( argvList, "-amos:", "debug", true ) ) {

			cout << "Enabling debugging mode for amos ..." << endl;
			enable_debug = true;
			CommandlineProcessing::removeArgsWithParameters( argvList, "-amos:debug" );
		}
		else
			enable_debug = false;

		if( CommandlineProcessing::isOption( argvList, "-amos:", "advice", true ) ) {

			cout << "Enabling advice mode for amos ..." << endl;
			enable_advice = true;
			CommandlineProcessing::removeArgsWithParameters( argvList, "-amos:advice" );
		}
		else
			enable_advice = false;

		if( CommandlineProcessing::isOption( argvList, "-amos:", "dump", true ) ) {

			cout << "Enabling dump mode for amos ..." << endl;
			enable_dump = true;
			CommandlineProcessing::removeArgsWithParameters( argvList, "-amos:dump" );
		}
		else
			enable_dump = false;

		if( CommandlineProcessing::isOption( argvList, "-amos:", "dumpall", true ) ) {

			cout << "Enabling dumpall mode for amos ..." << endl;
			enable_dump = true; // must to enable or amos_clear function will be executed
			enable_dumpall = true;
			CommandlineProcessing::removeArgsWithParameters( argvList, "-amos:dumpall" );
		}
		else
			enable_dumpall = false;

		if( (CommandlineProcessing::isOption( argvList, "--help", "", true )) ||
				(CommandlineProcessing::isOption( argvList, "-help", "", true )) ||
				(CommandlineProcessing::isOption( argvList, "-h", "", true )) ) {

			cout << "This is Amos' OpenMP task vaildation system:" << endl;
			cout << "Usage: ./amos [options] file..." << endl;
			cout << "Options:                       " << endl;
			cout << "\t--help            Display this information     " << endl;
			cout << "\t-help, -h                                         " << endl;
			cout << "\t--version         Display amos compiler information     " << endl;
			cout << "\t-version, -v                                       " << endl;
			cout << "\t-amos:advice      Give user advice on OpenMP task usage" << endl;
			cout << "\t-amos:debug       Run task validation system in a debugging mode" << endl;
			cout << "\t-amos:dump        Show transformation process file in /amos_tmp" << endl;
			cout << "\t-amos:dumpall     Leave all transformed file in this directory" << endl;
			cout << "-----------------------------------------------------------------------------" << endl;

			CommandlineProcessing::removeArgsWithParameters( argvList, "--help" );
			CommandlineProcessing::removeArgsWithParameters( argvList, "-help" );
			CommandlineProcessing::removeArgsWithParameters( argvList, "-h" );
		}

		if( (CommandlineProcessing::isOption( argvList, "--version", "", true )) ||
				(CommandlineProcessing::isOption( argvList, "-version", "", true )) ||
				(CommandlineProcessing::isOption( argvList, "-v", "", true )) ) {

			cout << "Some information about Amos' OpenMP task vaildation system:   " << endl;
			cout << "It's an OpenMP task vaildation system                         " << endl;
			cout << "Input: an OpenMP program.                                     " << endl;
			cout << "Output: the correct OpenMP program of insert proposed         " << endl; 
			cout << "        OpenMP directive.                                     " << endl;
			cout << "        Output file will be named as amos_[filename]          " << endl;
			cout << "                                                              " << endl;
			cout << "Features:                                                     " << endl;
			cout << "\t-Base on OpenMP tasking model.                              " << endl;
			cout << "\t-Only support tied task(no untied task).                    " << endl;
			cout << "\t-Cannot mix with OpenMP parallel and worksharing            " << endl;
			cout << "\t construct.                                                 " << endl;
			cout << "\t-Worst case is making execution as same as                  " << endl;
			cout << "\t serial execution.                                          " << endl;
			cout << "                                                              " << endl;
			cout << "Date: Aug 01, 2011                                            " << endl;
			cout << "editor: Amos Wang                                             " << endl;
			cout << "E-mail: amos76530@gmail.com                                   " << endl;
			cout << "HPC lab, csie, CCU                                            " << endl;

			CommandlineProcessing::removeArgsWithParameters( argvList, "--version" );
			CommandlineProcessing::removeArgsWithParameters( argvList, "-version" );
			CommandlineProcessing::removeArgsWithParameters( argvList, "-v" );
		}

		CommandlineProcessing::removeArgsWithParameters( argvList, "-amos:" );

	}// end command_processing

	bool parse_OmpTask( std::vector<std::string> &argvList )
	{
		SgProject *project = frontend( argvList );
		ROSE_ASSERT( project != NULL );

		if( enable_debug ) {

			cout << "parse_OmpTask command line list:" << endl;
			for( int i=0; i < argvList.size(); i++ ) {
				cout << argvList[i] << " ";
			}
			cout << endl;
		}

#if 1
		VariantVector vv( V_SgForStatement );
		Rose_STL_Container<SgNode*> loops = NodeQuery::queryMemoryPool(vv);
		for( Rose_STL_Container<SgNode*>::iterator iter = loops.begin(); iter!= loops.end(); iter++ ) {
			SgForStatement* cur_loop = isSgForStatement(*iter);
			ROSE_ASSERT(cur_loop);
			SageInterface::normalizeForLoopInitDeclaration(cur_loop);
		}
#endif

		initialize_analysis( project, false );

		//For each source file in the project
		SgFilePtrList &ptr_list = project->get_fileList();

		for( SgFilePtrList::iterator iter = ptr_list.begin(); iter != ptr_list.end(); iter++ ) {

			cout << "Source code: " << (*iter)->get_file_info()->get_filename() << endl;
			SgFile *sageFile = (*iter);
			SgSourceFile *sfile = isSgSourceFile(sageFile);
			ROSE_ASSERT(sfile);
			SgGlobal *root = sfile->get_globalScope();
			SgDeclarationStatementPtrList& declList = root->get_declarations ();
			int NumTasks = 0; //count number of tasks

			//For each function body in the scope
			for( SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p ) 
			{
				SgFunctionDeclaration *func = isSgFunctionDeclaration(*p);
				if ( func == 0 )  continue;

				SgFunctionDefinition *defn = func->get_definition();
				if ( defn == 0 )  continue;

				// Ignore functions in system headers, Can keep them to test robustness
				if ( defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename() )
					continue;

				SgBasicBlock *body = defn->get_body();

				// For each task
				Rose_STL_Container<SgNode*> tasks = NodeQuery::querySubTree( defn, V_SgOmpTaskStatement ); 

				//cout << "Num of OpenMP tasks: " << tasks.size() << endl;
				if ( tasks.size() == 0 ) {

					continue;
				}
				else {

					NumTasks += tasks.size();
					hasTask = true;
					SgIntVal *intVal = buildIntVal(0);
					SgVariableDeclaration *variableDeclaration =
						buildVariableDeclaration( "__Amos_Wang__", buildIntType(), buildAssignInitializer( intVal ) );
					prependStatement( variableDeclaration, body );
				}

				for ( Rose_STL_Container<SgNode*>::iterator iter = tasks.begin(); iter!= tasks.end(); iter++ ) {

					SgNode* current_task = *iter;
					if( enable_debug ) {
						cout << current_task->sage_class_name() << " at " << current_task  << " = "
							<< current_task->unparseToString().c_str() << endl;
						cout << "Task at line: " << current_task->get_file_info()->get_line() 
							<< ", " << current_task->get_file_info()->get_col() << endl;
					}
				}// end for loops

			}//end loop for each function body

			if( NumTasks == 0 ) 
				cout << "There is no OpenMP tasks!" << endl;
			else
				cout << "We found OpenMP tasks: " << NumTasks << endl;

		}//end loop for each source file

		release_analysis();

		if( hasTask )
			backend( project );

		return hasTask;
	}// end parse_OmpTask

	bool transform_Task2Loop( std::vector<std::string> &argvList )
	{
		ifstream infile;
		ofstream outfile;
		string line;
		size_t found;

		if( enable_debug )
			cout << "******** transform_Task2Loop *********" << endl;

		for( int i=0; i < argvList.size(); i++ ) {

			if( CommandlineProcessing::isSourceFilename(argvList[i]) ) {

				infile.open( ("rose_"+argvList[i]).c_str() );
				if( !infile ) {
					cerr << "error: unable to open " << ("rose_"+argvList[i]).c_str() << endl;
					return false;
				}

				outfile.open( ("temp_"+argvList[i]).c_str() );
				if( !outfile ) {
					cerr << "error: unable to open " << ("temp_"+argvList[i]).c_str() << endl;
					return false;
				}

				while( getline( infile, line ) ) {

					found = line.find( "#pragma omp taskwait" );
					if( found != string::npos )
						outfile << "//#pragma omp taskwait" << endl;
					else {

						found = line.find( "#pragma omp task" );
						if( found != string::npos )
							outfile << "for (__Amos_Wang__ = 0; __Amos_Wang__ < 1; __Amos_Wang__++)" << endl;
						else
							outfile << line << endl;
					}
				}

				infile.close();
				outfile.close();

				argvList[i] = "temp_" + argvList[i];
			}
			//cout << argvList[i] << endl;
		}

		return true;
	}// end transform_Task2Loop()

	bool amos_filter( std::vector<std::string> &argvList )
	{
		//cout << "begin amos_filter()" << endl;

		ifstream infile;
		ofstream outfile;
		string line;
		size_t found;

		cout << "Amos' Output: ";

		for( int i=0; i < argvList.size(); i++ ) {

			if( CommandlineProcessing::isSourceFilename(argvList[i]) ) {

				infile.open( ("rose_temp_"+argvList[i]).c_str() );
				if( !infile ) {
					cerr << "error: unable to open " << ("rose_temp"+argvList[i]).c_str() << endl;
					return false;
				}

				outfile.open( ("amos_"+argvList[i]).c_str() );
				if( !outfile ) {
					cerr << "error: unable to open " << ("amos_"+argvList[i]).c_str() << endl;
					return false;
				}

				while( getline( infile, line ) ) {

					found = line.find( "__Amos_Wang__" );
					if( found != string::npos ) {

						size_t found_for;
						found_for = line.find( "for" );

						if( found_for != string::npos ) {
							size_t found_bracket;

							found_bracket = line.find( "{" );

							if( found_bracket != string::npos )
								outfile << "  {" << endl;
						}
					}
					else
						outfile << line << endl;
				}

				infile.close();
				outfile.close();

				cout << ("amos_" + argvList[i]).c_str() << " ";
			}
			//cout << argvList[i] << endl;
		}
		cout << endl;

		if( !enable_dumpall || !enable_dump )
			amos_clear( argvList );

		//cout << "end amos_filter()" << endl;

		return true;
	}// end amos_filter()

	bool amos_clear( std::vector<std::string> &argvList )
	{
		std::string command;

		std::string key;
		std::string obj_name;
		size_t found;

		system( "rm -rf temp_*.o" );

		system( "mkdir -p amos_tmp" );

		for( int i=0; i < argvList.size(); i++ ) {

			if( CommandlineProcessing::isSourceFilename(argvList[i]) ) {

				obj_name = argvList[i];

				key = ".cpp";
				found = obj_name.rfind( key );
				if( found != string::npos ) {
					obj_name.replace( found, key.length(), ".o" );
				}
				else {
					key = ".CPP";
					found = obj_name.rfind( key );
					if( found != string::npos ) {
						obj_name.replace( found, key.length(), ".o" );
					}
					else {
						key = ".c";
						found = obj_name.rfind( key );
						if( found != string::npos ) 
							obj_name.replace( found, key.length(), ".o" );

						key = ".C";
						found = obj_name.rfind( key );
						if( found != string::npos ) 
							obj_name.replace( found, key.length(), ".o" );
					}
				}

				command = "rm -rf " + obj_name;
				system( command.c_str() );
				command = "mv rose_" + argvList[i] + " ./amos_tmp/";
				system( command.c_str() );
				command = "mv temp_" + argvList[i] + " ./amos_tmp/";
				system( command.c_str() );
				command = "mv rose_temp_" + argvList[i] + " ./amos_tmp/";
				system( command.c_str() );
			}
		}
		command = "rm a.out";
		system( command.c_str() );

		if( enable_dump )
			system( "cd amos_tmp; echo -n 'Amos Temp Directory is ';pwd" );
		else
			system( "rm -rf amos_tmp" );

		return true;
	}

	bool initialize_analysis( SgProject *project, bool debug )
	{
		if( enable_debug )
			cout << "Hello~ initialize_analysis" << endl;

		//Prepare def-use analysis
		if( defuse == NULL ) {
			ROSE_ASSERT( project != NULL );
			defuse = new DefUseAnalysis(project);
		}

		ROSE_ASSERT( defuse != NULL );

		int result = defuse->run( debug );

		if( result == 1 )
			cerr << "Error in Def-use analysis!" << endl;

		if( debug )
			defuse->dfaToDOT();

		//Prepare variable liveness analysis
		if( liv == NULL ) {
			liv = new LivenessAnalysis( debug, (DefUseAnalysis*) defuse );
		}
		ROSE_ASSERT( liv != NULL );

		std::vector <FilteredCFGNode <IsDFAFilter> > dfaFunctions;
		NodeQuerySynthesizedAttributeType vars = 
			NodeQuery::querySubTree( project, V_SgFunctionDefinition );
		NodeQuerySynthesizedAttributeType::const_iterator i;
		bool abortme = false;

		// run liveness analysis on each function body
		for ( i = vars.begin(); i != vars.end(); ++i ) 
		{
			SgFunctionDefinition* func = isSgFunctionDefinition(*i);
			if (debug)
			{
				std::string name = func->class_name();
				string funcName = func->get_declaration()->get_qualified_name().str();
				cout<< " .. running liveness analysis for function: " << funcName << endl;
			}

			// cause error message: Bad statement case SgOmpTaskwaitStatement in cfgIndexForEnd()
			FilteredCFGNode <IsDFAFilter> rem_source = liv->run(func,abortme);
			if (rem_source.getNode()!=NULL)
				dfaFunctions.push_back(rem_source);    
			if (abortme)
				break;
		} // end for ()

		if(debug)
		{
			cout << "Writing out liveness analysis results into var.dot... " << endl;
			std::ofstream f2("var.dot");
			dfaToDot(f2, string("var"), dfaFunctions, (DefUseAnalysis*)defuse, liv);
			f2.close();
		}
		if (abortme) {
			cerr<<"Error: Liveness analysis is ABORTING ." << endl;
			ROSE_ASSERT(false);
		}
		return !abortme;
	}// end initialize_analysis

	void release_analysis()
	{
		if( enable_debug )
			cout << "Hello~ release_analysis" << endl;

		if( defuse != NULL ) {
			delete defuse;
			defuse = NULL;
		}
		if( liv != NULL ) {
			delete liv;
			liv = NULL;
		}
	}// end release_analysis

	SgInitializedName* getLoopInvariant(SgNode* loop)
	{
		AstInterfaceImpl faImpl(loop);
		AstInterface fa(&faImpl);
		AstNodePtr ivar2 ;
		AstNodePtrImpl loop2(loop);
		bool result = fa.IsFortranLoop(loop2, &ivar2);

		if ( !result ) return NULL;

		SgVarRefExp* invar = isSgVarRefExp( AstNodePtrImpl(ivar2).get_ptr() );
		ROSE_ASSERT(invar);

		SgInitializedName* invarname = invar->get_symbol()->get_declaration();

		/*
		   if( enable_debug )
		   cout<< "getLoopInvariant name:" << invarname->get_name().getString() << endl;
		 */

		return invarname;
	}// end getLoopInvariant

	bool TaskValidation( SgNode *loop )
	{
		ROSE_ASSERT( loop );
		ROSE_ASSERT( isSgForStatement(loop) );


		SgNode *task = loop;
		OmpSupport::OmpAttribute *omp_attribute = buildOmpAttribute( e_unknown, NULL, false );
		ROSE_ASSERT( omp_attribute != NULL );

		TaskAutoScoping( task, omp_attribute );

		omp_attribute->setOmpDirectiveType( OmpSupport::e_task );

		OmpSupport::addOmpAttribute(omp_attribute, task);

		OmpSupport::generatePragmaFromOmpAttribute( task );

		return true;
	}

	/* You can enhance more analysis information in this function! */
	void TaskAutoScoping( SgNode *sg_node, OmpSupport::OmpAttribute *attribute )
	{
		//cout << "Enter AutoScoping" << endl;

		ROSE_ASSERT( sg_node && attribute );
		ROSE_ASSERT( isSgForStatement(sg_node) );

		// Variable liveness analysis: original ones and 
		std::vector<SgInitializedName *> liveIns0, liveIns;
		std::vector<SgInitializedName *> liveOuts0, liveOuts;

		// get information from Liveness analysis
		GetLiveVariables( sg_node, liveIns0, liveOuts0, false );

		sort( liveIns0.begin(), liveIns0.end() );
		sort( liveOuts0.begin(), liveOuts0.end() );

		std::vector<SgInitializedName *> allVars, 
			inputVars, innerVars, outputVars, 
			defVars, temp;

		CollectVisibleVariables( sg_node, allVars, true );

		if( enable_debug ) {

			cout << "Debug allVars after CollectVariables():" << endl;
			for( std::vector<SgInitializedName*>::iterator iter = allVars.begin(); iter != allVars.end(); iter++ ) {
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString()<<endl;
			}
		}

		// get information from def-use analysis
		CollectDefVariables( sg_node, defVars );

		if( enable_debug ) {

			cout << "Debug defVars after CollectDefVariables():" << endl;
			for( std::vector<SgInitializedName*>::iterator iter = defVars.begin(); iter != defVars.end(); iter++ ) {
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString()<<endl;
			}
		}

		set_intersection( liveIns0.begin(), liveIns0.end(), allVars.begin(), allVars.end(),
				inserter( liveIns, liveIns.begin() ) );

		set_intersection( liveOuts0.begin(), liveOuts0.end(), allVars.begin(), allVars.end(),
				inserter( liveOuts, liveOuts.begin() ) );

		sort( liveIns.begin(), liveIns.end() );
		sort( liveOuts.begin(), liveOuts.end() );

		if( enable_debug ) {

			cout << "Debug liveIns:" << endl;
			for( std::vector<SgInitializedName*>::iterator iter = liveIns.begin(); iter != liveIns.end(); iter++ ) {
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString()<<endl;
			}

			cout << "Debug liveOuts:" << endl;
			for( std::vector<SgInitializedName*>::iterator iter = liveOuts.begin(); iter != liveOuts.end(); iter++ ) {
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString()<<endl;
			}

		}


		//shared: outputVars
		//---------------------------------------------
		//liveOuts && defVars

		set_intersection( liveOuts.begin(), liveOuts.end(), defVars.begin(), defVars.end(),
				inserter( outputVars, outputVars.begin() ) );

		sort( outputVars.begin(), outputVars.end() );

		/*
		   for( std::vector<SgInitializedName *>::iterator iter = liveOuts.begin(); iter != liveOuts.end(); iter++ ) {
		   outputVars.push_back( *iter );
		   }
		 */

		if( enable_debug ) {
			cout << "Debug dump outputVars:" << endl;
		}
		else if( enable_advice ) {

			if( outputVars.size() != 0 ) 
				cout << "OutputVars should be shared variable:" << endl;
		}
		else
			;

		for( std::vector<SgInitializedName *>::iterator iter = outputVars.begin(); iter != outputVars.end(); iter++ ) {

			attribute->addVariable( OmpSupport::e_shared, (*iter)->get_name().getString(), *iter );

			if( enable_debug )
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString() << endl;
			else if( enable_advice )
				cout << (*iter)->get_qualified_name().getString() << endl;
			else
				;
		}

		if( outputVars.size() != 0 ) {

			//cout << "There are shared variables therefore we need synchornization." << endl;
			add_synchronization( sg_node );
		}


		//firstprivate: inputVars
		//---------------------------------------------
		//liveIns - outputVars 

		set_difference( liveIns.begin(), liveIns.end(), outputVars.begin(), outputVars.end(), 
				inserter( inputVars, inputVars.begin() ) );

		if( enable_debug ) {
			cout << "Debug dump inputVars:" << endl;
		}
		else if( enable_advice ) {

			if( inputVars.size() != 0 ) 
				cout << "InputVars should be firstprivate variable:" << endl;
		}
		else
			;

		for( std::vector<SgInitializedName *>::iterator iter = inputVars.begin(); iter != inputVars.end(); iter++ ) {

			attribute->addVariable( OmpSupport::e_firstprivate, (*iter)->get_name().getString(), *iter );

			if( enable_debug )
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString() << endl;
			else if( enable_advice )
				cout << (*iter)->get_qualified_name().getString() << endl;
			else
				;
		}


		//private: innerVars
		//---------------------------------------------
		//allVars - liveIns - liveOuts

		set_union( liveIns.begin(), liveIns.end(), liveOuts.begin(), liveOuts.end(), 
				inserter( temp, temp.begin() ) );

		set_difference( allVars.begin(), allVars.end(), temp.begin(), temp.end(), 
				inserter( innerVars, innerVars.begin() ) );

		if( enable_debug ) {
			cout << "Debug dump innerVars:" << endl;
		}
		else if( enable_advice ) {

			if( innerVars.size() != 0 ) 
				cout << "InnerVars should be private variable:" << endl;
		}
		else
			;

		for( std::vector<SgInitializedName *>::iterator iter = innerVars.begin(); iter != innerVars.end(); iter++ ) {

			attribute->addVariable( OmpSupport::e_private, (*iter)->get_name().getString(), *iter );

			if( enable_debug )
				cout << (*iter) << " " << (*iter)->get_qualified_name().getString() << endl;
			else if( enable_advice )
				cout << (*iter)->get_qualified_name().getString() << endl;
			else
				;
		}

		//cout << "Leave AutoScoping" << endl;
	}// end AutoScoping()

	void add_synchronization( SgNode *sg_node )
	{
		ROSE_ASSERT( sg_node );

		OmpSupport::OmpAttribute *taskwait_attribute = buildOmpAttribute( e_unknown, NULL, false );
		ROSE_ASSERT( taskwait_attribute != NULL );

		taskwait_attribute->setOmpDirectiveType( OmpSupport::e_taskwait );

		//OmpSupport::addOmpAttribute(taskwait_attribute, sg_node);

		SgStatement* cur_stmt = isSgStatement(sg_node);
		ROSE_ASSERT(cur_stmt != NULL);

		SgPragmaDeclaration * pragma = SageBuilder::buildPragmaDeclaration("omp taskwait");
		SageInterface::insertStatementAfter(cur_stmt, pragma);

	}// end add_synchronization()

	void GetLiveVariables( SgNode *loop, std::vector<SgInitializedName*> &liveIns, 
			std::vector<SgInitializedName*> &liveOuts, bool reCompute/*=false*/ )
	{
		//reCompute : call another liveness analysis function on a target function
		if( reCompute )
			initialize_analysis();

		//store the original one
		std::vector<SgInitializedName*> liveIns0, liveOuts0;

		SgInitializedName* invarname = getLoopInvariant(loop);

		//Grab the filtered CFG node for SgForStatement
		SgForStatement *forstmt = isSgForStatement(loop);
		ROSE_ASSERT( forstmt );

		//Several CFG nodes are used for the same SgForStatement
		CFGNode cfgnode( forstmt, 2 );
		FilteredCFGNode<IsDFAFilter> filternode = FilteredCFGNode<IsDFAFilter> (cfgnode);

		//This one does not return the one we want even its getNode returns the right for statement
		//FilteredCFGNode<IsDFAFilter> filternode= FilteredCFGNode<IsDFAFilter> (forstmt->cfgForBeginning());
		ROSE_ASSERT( filternode.getNode() == forstmt );

		// Check out edges
		vector<FilteredCFGEdge < IsDFAFilter > > out_edges = filternode.outEdges();
		//cout<<"Found edge count:"<<out_edges.size()<<endl;

		//SgForStatement should have two outgoing edges, one true(going into the loop body) and one false (going out the loop)
		//ROSE_ASSERT( out_edges.size() == 2 ); 
		vector<FilteredCFGEdge < IsDFAFilter > >::iterator iter = out_edges.begin();

		//  std::vector<SgInitializedName*> remove1, remove2;
		for ( ; iter != out_edges.end(); iter++ ) {

			FilteredCFGEdge<IsDFAFilter> edge = *iter;

			// Used to verify CFG nodes in var.dot dump
			//x. Live-in (loop) = live-in (first-stmt-in-loop)
			if( edge.condition() == eckTrue ) {

				SgNode* firstnode = edge.target().getNode();
				liveIns0 = liv->getIn( firstnode );

				//if( enable_debug )
				//	cout << "Live-in variables for loop:" << endl;

				// collect all Live-in vaialbes within the loop
				for ( std::vector<SgInitializedName*>::iterator iter = liveIns0.begin(); iter!=liveIns0.end(); iter++ ) {

					SgInitializedName *name = *iter;

					if( ( SageInterface::isScalarType(name->get_type()) ) && ( name != invarname ) ) {

						liveIns.push_back(*iter);
						//remove1.push_back(*iter);

						//if( enable_debug )
						//	cout << name->get_qualified_name().getString() << endl;
					}
				}
			}//x. live-out(loop) = live-in (first-stmt-after-loop)
			else if( edge.condition() == eckFalse ) {

				SgNode* firstnode = edge.target().getNode();
				liveOuts0 = liv->getIn(firstnode);

				//if( enable_debug )
				//	cout << "Live-out variables for loop:" << endl;

				for( std::vector<SgInitializedName*>::iterator iter = liveOuts0.begin(); iter!=liveOuts0.end(); iter++ ) {

					SgInitializedName* name = *iter;

					if( ( SageInterface::isScalarType(name->get_type()) ) && ( name != invarname ) ) {

						liveOuts.push_back( *iter );
						//remove2.push_back(*iter);
						//if( enable_debug )
						//	cout << name->get_qualified_name().getString() << endl;
					}
				}
			}
			else {

				cerr << "Unexpected CFG out edge type for SgForStmt!" << endl;
				ROSE_ASSERT( false );
			}
		} //end for (edges)

		//debug the final results
		if( enable_debug ) {

			cout << "Final Live-in variables for loop:" << endl;

			for( std::vector<SgInitializedName*>::iterator iter = liveIns.begin(); iter != liveIns.end(); iter++ ) {

				SgInitializedName* name = *iter;
				cout << name->get_qualified_name().getString() << endl;
			}

			cout << "Final Live-out variables for loop:" << endl;

			for( std::vector<SgInitializedName*>::iterator iter = liveOuts.begin(); iter != liveOuts.end(); iter++ ) {

				SgInitializedName* name = *iter;
				cout << name->get_qualified_name().getString() << endl;
			}
		}

	} // end GetLiveVariables()

	void CollectDefVariables( SgNode *loop, std::vector<SgInitializedName*> &resultVars )
	{
		ROSE_ASSERT(loop !=NULL);

		//Get the scope of the loop
		SgScopeStatement* currentscope = isSgFunctionDeclaration(\
				SageInterface::getEnclosingFunctionDeclaration(loop))\
						 ->get_definition()->get_body();

		ROSE_ASSERT(currentscope != NULL);

		SgInitializedName* invarname = getLoopInvariant(loop);

		Rose_STL_Container<SgNode*> reflist = NodeQuery::querySubTree(loop, V_SgVarRefExp);

		for ( Rose_STL_Container<SgNode*>::iterator i = reflist.begin(); i != reflist.end(); i++ ) {

			SgVarRefExp* varRef = isSgVarRefExp(*i);
			SgInitializedName* initname = isSgVarRefExp(*i)->get_symbol()->get_declaration();
			SgScopeStatement* varscope = initname->get_scope();

			// find reaching definition of initName at the control flow node varRef
			vector<SgNode*> vec = defuse->getDefFor( varRef, initname );
			//ROSE_ASSERT(vec.size()>0);

			//cout << vec.size() << " for " << varRef->unparseToString() << endl;
			// only collect variables which are visible at the loop's scope
			// varscope is equal or higher than currentscope 
			if( ( currentscope == varscope ) || ( SageInterface::isAncestor(varscope,currentscope) ) ) { 

				if( invarname != initname ) {
					for( size_t j = 0; j < vec.size(); j++ ) {

						//cout << vec[j]->class_name() << " " << vec[j] << endl;
						// special case about SgPlusPlusOp :this case must skip unless program will quit!
						if( vec[j]->class_name().compare("SgPlusPlusOp") == 0 ) {
							resultVars.push_back(initname);
							continue;
						}

						SgStatement* def_stmt = SageInterface::getEnclosingStatement( vec[j] );
						ROSE_ASSERT( def_stmt );
						SgScopeStatement* defscope = def_stmt->get_scope();

						// in the same omptask scope
						if( currentscope != defscope || varscope != defscope ) {
							resultVars.push_back(initname);
							//cout << "Def: [" << def_stmt->unparseToString() << "] @ line " 
							//<< def_stmt->get_file_info()->get_line() 
							//<< ":" << def_stmt->get_file_info()->get_col() << endl;
						}
					}
				}
			}
		} // end for()

		//Remove duplicated items 
		sort(resultVars.begin(),resultVars.end()); 
		std::vector<SgInitializedName*>::iterator new_end= unique(resultVars.begin(),resultVars.end());
		resultVars.erase(new_end, resultVars.end());
	}

	void CollectVisibleVariables( SgNode *loop, std::vector<SgInitializedName*> &resultVars, bool scalarOnly/*=false*/ )
	{
		ROSE_ASSERT(loop !=NULL);

		//Get the scope of the loop
		SgScopeStatement* currentscope = isSgFunctionDeclaration(\
				SageInterface::getEnclosingFunctionDeclaration(loop))\
						 ->get_definition()->get_body();

		ROSE_ASSERT(currentscope != NULL);

		SgInitializedName* invarname = getLoopInvariant(loop);

		Rose_STL_Container<SgNode*> reflist = NodeQuery::querySubTree(loop, V_SgVarRefExp);

		for ( Rose_STL_Container<SgNode*>::iterator i = reflist.begin(); i != reflist.end(); i++ ) {

			SgInitializedName* initname = isSgVarRefExp(*i)->get_symbol()->get_declaration();
			SgScopeStatement* varscope = initname->get_scope();
			// only collect variables which are visible at the loop's scope
			// varscope is equal or higher than currentscope 
			if( ( currentscope == varscope ) || ( SageInterface::isAncestor(varscope,currentscope) ) ) { 
				// Skip non-scalar if scalarOnly is requested
				if( (scalarOnly) && !SageInterface::isScalarType(initname->get_type()) )
					continue;
				if( invarname != initname )  
					resultVars.push_back(initname);
			}
		} // end for()

		//Remove duplicated items 
		sort(resultVars.begin(),resultVars.end()); 
		std::vector<SgInitializedName*>::iterator new_end= unique(resultVars.begin(),resultVars.end());
		resultVars.erase(new_end, resultVars.end());

	}// end CollectVisibleVaribles

	long usecs (void)
	{
		struct timeval t;

		gettimeofday( &t, NULL );

		return t.tv_sec*1000000+t.tv_usec;
	}


}// end namespace

