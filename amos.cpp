/*Chun-Kun Wang (amos@cs.unc.edu)*/

#include "rose.h"
#include "amosValidation.h"

#include <iostream>
#include <vector>
#include <string>

using namespace std;
using namespace AmosValidation;

int main( int argc, char *argv[] )
{
	if( argc < 2 ) {
		cout << "./amos: no input files                               " << endl;
		cout << "                                                     " << endl;
		cout << "Hi, this is Amos!                                    " << endl;
		cout << "It's my pleasure to serve you.                       " << endl;
		cout << "                                                     " << endl;
		cout << "Please type option '--help' to see guide             " << endl;
		cout << "                                                     " << endl;

		return 0;
	}

	cout << "*************************************************************" << endl;
	cout << "**                                                         **" << endl;
	cout << "**      Welcome to use OpenMP task validation system!      **" << endl;
	cout << "**                                                         **" << endl;
	cout << "**                                      editor: Amos Wang  **" << endl;
	cout << "*************************************************************\n" << endl;

	vector<string> argvList( argv, argv+argc );
	vector<string> argvList0( argv, argv+argc ); // keep original argv and argc

	command_processing( argvList );

	if( !parse_OmpTask( argvList ) ) {

		cout << "\nAmos says: I am sorry that I could not find any OpenMP task !" << endl << endl;
		return 0;
	}

	// for time counter
	long t_start;
	long t_end;
	double time_program = 0.0;

	t_start = usecs(); 
	// for time counter

	transform_Task2Loop( argvList );

	SgProject *project = frontend(argvList);
	ROSE_ASSERT( project != NULL );

#if 1
	VariantVector vv( V_SgForStatement );
	Rose_STL_Container<SgNode*> loops = NodeQuery::queryMemoryPool(vv);
	for( Rose_STL_Container<SgNode*>::iterator iter = loops.begin(); iter!= loops.end(); iter++ ) {
		SgForStatement* cur_loop = isSgForStatement(*iter);
		ROSE_ASSERT(cur_loop);
		SageInterface::normalizeForLoopInitDeclaration(cur_loop);
	}
#endif

	//initialize_analysis( project, false );
	initialize_analysis( project, false );

	//For each source file in the project
	SgFilePtrList &ptr_list = project->get_fileList();

	cout << "\n**** Amos' validation system running ****\n" << endl;

	for( SgFilePtrList::iterator iter = ptr_list.begin(); iter != ptr_list.end(); iter++ ) {

		cout << "temp source code: " << (*iter)->get_file_info()->get_filename() << endl << endl;
		SgFile *sageFile = (*iter);
		SgSourceFile *sfile = isSgSourceFile(sageFile);
		ROSE_ASSERT(sfile);
		SgGlobal *root = sfile->get_globalScope();
		SgDeclarationStatementPtrList& declList = root->get_declarations ();

		//cout << "Check point 2" << endl;
		//For each function body in the scope
		for( SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p ) 
		{
			SgFunctionDeclaration *func = isSgFunctionDeclaration(*p);
			if ( func == 0 )  continue;

			SgFunctionDefinition *defn = func->get_definition();
			if ( defn == 0 )  continue;

			//ignore functions in system headers, Can keep them to test robustness
			if ( defn->get_file_info()->get_filename() != sageFile->get_file_info()->get_filename() )
				continue;

			SgBasicBlock *body = defn->get_body();

			// For each loop
			Rose_STL_Container<SgNode*> loops = NodeQuery::querySubTree( defn, V_SgForStatement ); 

			if( loops.size() == 0 ) continue;

			// X. Replace operators with their equivalent counterparts defined 
			// in "inline" annotations
			AstInterfaceImpl faImpl_1( body );
			CPPAstInterface fa_body( &faImpl_1 );
			OperatorInlineRewrite()( fa_body, AstNodePtrImpl(body) );

			// Pass annotations to arrayInterface and use them to collect 
			// alias info. function info etc.  
			ArrayAnnotation* annot = ArrayAnnotation::get_inst(); 
			ArrayInterface array_interface( *annot );
			array_interface.initialize( fa_body, AstNodePtrImpl(defn) );
			array_interface.observe( fa_body );

			// X. Loop normalization for all loops within body
			NormalizeForLoop(fa_body, AstNodePtrImpl(body));

			//cout << "Check point 3" << endl;
			for ( Rose_STL_Container<SgNode*>::iterator iter = loops.begin(); iter!= loops.end(); iter++ ) {

				SgNode* current_loop = *iter;
				SgInitializedName* invarname = getLoopInvariant( current_loop );

				if( invarname != NULL ) {

					if( invarname->get_name().getString().compare("__Amos_Wang__") == 0 ) {

						//cout << "It's __Amos_Wang__." << endl;
						//replace "for(__Amos_Wang__ = 0;__Amos_Wang__ <= 1 - 1;__Amos_Wang__ += 1)" 
						//to "#pragma omp task"
						std::string strtemp = current_loop->unparseToString();
						strtemp.replace( 0, 64, "#pragma omp task" );

						cout << "task position at: " << current_loop->get_file_info()->get_line()
							<< ", " << current_loop->get_file_info()->get_col() << endl;

						cout << "context: " << strtemp.c_str() << endl; 

						TaskValidation( current_loop );

						cout << "TaskValidation done...\n" << endl;
					}
					else continue;

				}

			}// end for loops

		}//end loop for each function body
		cout << "--------------------------------------------" << endl;

	}//end loop for each source file

	release_analysis();

	//generateDOT( *project );
	backend( project );

	//generate final file with correct directive
	amos_filter( argvList0 );

	// for time counter
	t_end = usecs();
	time_program = ((double)(t_end - t_start))/1000000;
	cout << "analysis time: " << time_program << " sec" << endl;
	//

	cout << endl << "***** Thank you for using Amos' compiler ! *****\n" << endl;

	return 0;
}

