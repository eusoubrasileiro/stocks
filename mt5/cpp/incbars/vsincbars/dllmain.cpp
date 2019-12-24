// python interpreter embedded in this dll
#include "dll.h"


// https://docs.python.org/3/c-api/init.html#non-python-created-threads
PyGILState_STATE gstate; // calling python from a thread not created by Python itself

bool calledbyPython=false; // if called by Python dll behaves differently

BOOL __stdcall DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
			debugfile << "PROCESS_ATTACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
			try {
				if (!Py_IsInitialized()) // starts only once on the mt5 process
					py::initialize_interpreter();
				else // just get the GIL for the thread who called the DLL
					gstate = PyGILState_Ensure();
                LoadPythonCode();
			}
			catch (py::error_already_set const& pythonErr){
				debugfile << "python exception: " << std::endl;
				debugfile << pythonErr.what() << std::endl;
			}
			catch (const std::exception& ex){
				debugfile << "c++ exception: " << std::endl;
				debugfile << ex.what() << std::endl;
			}
			catch (...){
				debugfile << "Weird no idea exception" << std::endl;
			}
		// attach to process
		// return FALSE to fail DLL load
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH: // will keep runing until mt5 dies or python dies
			debugfile << "PROCESS_DETACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
		// Solved many booms $&##��*�@!(@ on metatrader 5
		// releasing the thread GIL (note that the main thread allways has one of this)
		PyGILState_Release(gstate);
        // only use when called as a python module
        if (Py_IsInitialized() && calledbyPython) { 
	        pycode.~module(); // must destroy here otherwise will try to destroy
            //after the interpreter is destroyed and booom!!!$&##��*�@!(@
	        py::finalize_interpreter();
	        debugfile << "finalized interpreter" << std::endl;
         }        
        //if(!calledbyPython)
        //    // print number of ticks processed if on backtest
        break;
    }
    return TRUE;
}

// kills the jupyter python interpreter
//void unloadModule(){
//    PyGILState_Release(gstate);
//    if (Py_IsInitialized()) {
//        pycode.~module(); // must destroy here otherwise will try to destroy
//        //after the interpreter is destroyed and booom!!!$&##��*�@!(@
//        py::finalize_interpreter();
//        debugfile << "finalized interpreter" << std::endl;
//    } // also kills the jupyter python kernel 
//}