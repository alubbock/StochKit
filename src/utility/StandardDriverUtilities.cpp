/*!

*/
#include "StandardDriverUtilities.h"

namespace STOCHKIT
{
	void StandardDriverUtilities::createOutputDirs(CommandLineInterface& commandLine, bool parallel, std::size_t threads)
	{  

		std::string outputDir=commandLine.getOutputDir();

		try {
			if (boost::filesystem::exists(outputDir)) {
				if (!commandLine.getForce()) {
					std::cout << "StochKit ERROR (StandardDriverUtilities::createOutputDirs): output directory \""<<outputDir<<"\" already exists.\n";
					std::cout << "Delete existing directory, use --out-dir to specify a unique directory name, or run with --force to overwrite.\n";
					std::cout << "Simulation terminated.\n";
					exit(1);
				}
				else {
					//delete existing directory
					if (boost::filesystem::is_directory(outputDir)) {
						//could do some checks here to ensure they're not deleting a StochKit directory such as "src", "libs", "models", etc.
						//currently, that is the risk one takes in using --force
						boost::filesystem::remove_all(outputDir);
					}
					//abort if it exists but is not a directory
					else {
						std::cerr << "StochKit ERROR (StandardDriverUtilities::createOutputDirs): output directory \""<<outputDir<<"\" exists but is not a directory.\n";
						std::cerr << "Delete existing file or use --out-dir to specify a unique directory name.\n";
						std::cerr << "Simulation terminated.\n";
						exit(1);
					}
				}
			}
			boost::filesystem::create_directories(outputDir);
#ifdef WIN32
			if (commandLine.getKeepStats()) {
				boost::filesystem::create_directory(outputDir+"\\"+commandLine.getStatsDir());
				if (parallel) {
					boost::filesystem::create_directory(outputDir+"\\"+commandLine.getStatsDir()+"\\.parallel");
				}
			}
			if (commandLine.getKeepTrajectories()) {
				boost::filesystem::create_directory(outputDir+"\\"+commandLine.getTrajectoriesDir());
			}
			if (commandLine.getKeepHistograms()) {
				boost::filesystem::create_directory(outputDir+"\\"+commandLine.getHistogramsDir());
				if (parallel) {
					boost::filesystem::create_directory(outputDir+"\\"+commandLine.getHistogramsDir()+"\\.parallel");	
					for (std::size_t i=0; i!=threads; ++i) {
						std::string threadNumStr=StandardDriverUtilities::size_t2string(i);
						boost::filesystem::create_directory(outputDir+"\\"+commandLine.getHistogramsDir()+"\\.parallel\\thread"+threadNumStr);	
					}
				}

			}
			//create a hidden directory for StochKit data (e.g. a file that lists the solver used, the parameters, maybe even a copy of the model file
			boost::filesystem::create_directory(outputDir+"\\.StochKit");      
#else
			if (commandLine.getKeepStats()) {
				boost::filesystem::create_directory(outputDir+"/"+commandLine.getStatsDir());
				if (parallel) {
					boost::filesystem::create_directory(outputDir+"/"+commandLine.getStatsDir()+"/.parallel");
				}
			}
			if (commandLine.getKeepTrajectories()) {
				boost::filesystem::create_directory(outputDir+"/"+commandLine.getTrajectoriesDir());
			}
			if (commandLine.getKeepHistograms()) {
				boost::filesystem::create_directory(outputDir+"/"+commandLine.getHistogramsDir());
				if (parallel) {
					boost::filesystem::create_directory(outputDir+"/"+commandLine.getHistogramsDir()+"/.parallel");	
					for (std::size_t i=0; i!=threads; ++i) {
						std::string threadNumStr=StandardDriverUtilities::size_t2string(i);
						boost::filesystem::create_directory(outputDir+"/"+commandLine.getHistogramsDir()+"/.parallel/thread"+threadNumStr);	
					}
				}

			}
			//create a hidden directory for StochKit data (e.g. a file that lists the solver used, the parameters, maybe even a copy of the model file
			boost::filesystem::create_directory(outputDir+"/.StochKit");
#endif
		}
		catch (...) {
			std::cerr << "StochKit ERROR (StandardDriverUtilities::createOutputDirs): error creating output directory.\n";
			exit(1);
		}
	}

#ifdef WIN32
	void StandardDriverUtilities::compileMixed(std::string executableName, const CommandLineInterface& commandLine, const boost::filesystem::path current_path, bool events) {
		std::string generatedCodeDir=current_path.parent_path().string()+"\\generatedCode";
		if (commandLine.getRecompile()) {
			try {
				if (boost::filesystem::exists(generatedCodeDir)) {
					//delete existing directory
					if (boost::filesystem::is_directory(generatedCodeDir)) {
						boost::filesystem::remove_all(generatedCodeDir);
					}
					//abort if it exists but is not a directory
					else {
						std::cerr << "StochKit ERROR (StandardDriverUtilities::compileMixed): generated code directory \""<<generatedCodeDir<<"\" already exists and is not a directory.\n";
						std::cerr << "Simulation terminated.\n";
						exit(1);
					}
				}
				boost::filesystem::create_directory(generatedCodeDir);
			}
			catch (...) {
				std::cerr << "StochKit ERROR (StandardDriverUtilities::compileMixed): error creating output directory.\n";
				exit(1);
			}

			char modelFileName[2048];

			std::string tempname;
                        char tempname_c_str[2048];
			tempname=commandLine.getModelFileName();
			strcpy(modelFileName, tempname.c_str());

#ifdef EVENTS
			Input_events_before_compile<StandardDriverTypes::populationType, 
				StandardDriverTypes::stoichiometryType,
				/*StandardDriverTypes::propensitiesType,*/
				StandardDriverTypes::graphType> model(modelFileName);
                        tempname = (generatedCodeDir+"\\CustomPropensityFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomPropensityFunctionsFile(tempname_c_str);
                        tempname = (generatedCodeDir+"\\CustomStateBasedTriggerFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomStateBasedTriggerFunctionsFile(tempname_c_str);
                        tempname = (generatedCodeDir+"\\CustomChangeSingleSpeciesFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomChangeSingleSpeciesFunctionsFile(tempname_c_str);
#else
			Input_mixed_before_compile<StandardDriverTypes::populationType, 
				StandardDriverTypes::stoichiometryType,
				StandardDriverTypes::propensitiesType,
				StandardDriverTypes::graphType> model(modelFileName);
                        tempname = (generatedCodeDir+"\\CustomPropensityFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomPropensityFunctionsFile(tempname_c_str);
#endif

			//record current path so we can cd back to it after compiling
			std::string currentPath=current_path.string();
			std::string pathName=current_path.parent_path().string()+"\\Mixed_Compiled_Solution\\Mixed_Compiled_Solution.sln";
			//redirect any errors from make to a log file
#ifdef NDEBUG
			std::string makeCommand=(std::string)"msbuild \""+pathName+"\" /t:"+executableName+":rebuild /clp:NoSummary /p:configuration=release;DebugType=none /v:q /nologo /flp:LogFile=\""+generatedCodeDir+"\\compile-log.txt\";Verbosity=diagnostic";
#else
			std::string makeCommand=(std::string)"msbuild \""+pathName+"\" /t:"+executableName+":rebuild /clp:NoSummary /p:configuration=debug /v:q /nologo /flp:LogFile=\""+generatedCodeDir+"\\compile-log.txt\";Verbosity=diagnostic";
#endif

			std::cout << "StochKit MESSAGE: compiling generated code...this will take a few moments...\n";
			int returnValue=system(makeCommand.c_str());

			if (returnValue!=0) {
				std::cout << "StochKit ERROR: compile of generated code failed.  Simulation terminated.\n";
				//copy hidden compile-log to visible log (this seens useless for windows)
			/*	std::string command="copy /B "+commandLine.getGeneratedCodeDir()+"\\.compile-log.txt "+commandLine.getGeneratedCodeDir()+"\\compile-log.txt";

				system(command.c_str());*/

				std::cout << "Check log file \"" << generatedCodeDir<<"\\compile-log.txt\" for error messages.\n";
				exit(1);
			}

			//go back to original path
			std::string cd="cd "+currentPath;
			system(cd.c_str());
		}
	}
#else
	void StandardDriverUtilities::compileMixed(std::string executableName, const CommandLineInterface& commandLine, bool events) {
		if (commandLine.getRecompile()) {
			try {
				if (boost::filesystem::exists(commandLine.getGeneratedCodeDir())) {
					//delete existing directory
					if (boost::filesystem::is_directory(commandLine.getGeneratedCodeDir())) {
						boost::filesystem::remove_all(commandLine.getGeneratedCodeDir());
					}
					//abort if it exists but is not a directory
					else {
						std::cerr << "StochKit ERROR (StandardDriverUtilities::compileMixed): generated code directory \""<<commandLine.getGeneratedCodeDir()<<"\" already exists and is not a directory.\n";
						std::cerr << "Simulation terminated.\n";
						exit(1);
					}
				}
				boost::filesystem::create_directory(commandLine.getGeneratedCodeDir());
				boost::filesystem::create_directory(commandLine.getGeneratedCodeDir()+"/bin");
			}
			catch (...) {
				std::cerr << "StochKit ERROR (StandardDriverUtilities::compileMixed): error creating output directory.\n";
				exit(1);
			}

			char modelFileName[2048];

			std::string tempname;
                        char tempname_c_str[2048];
			tempname=commandLine.getModelFileName();
			strcpy(modelFileName, tempname.c_str());

#ifdef EVENTS
			Input_events_before_compile<StandardDriverTypes::populationType, 
				StandardDriverTypes::stoichiometryType,
				/*StandardDriverTypes::propensitiesType,*/
				StandardDriverTypes::graphType> model(modelFileName);
                        tempname = (commandLine.getGeneratedCodeDir()+"/CustomPropensityFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomPropensityFunctionsFile(tempname_c_str);
                        tempname = (commandLine.getGeneratedCodeDir()+"/CustomStateBasedTriggerFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomStateBasedTriggerFunctionsFile(tempname_c_str);
                        tempname = (commandLine.getGeneratedCodeDir()+"/CustomChangeSingleSpeciesFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomChangeSingleSpeciesFunctionsFile(tempname_c_str);
#else
			Input_mixed_before_compile<StandardDriverTypes::populationType, 
				StandardDriverTypes::stoichiometryType,
				StandardDriverTypes::propensitiesType,
				StandardDriverTypes::graphType> model(modelFileName);
                        tempname = (commandLine.getGeneratedCodeDir()+"/CustomPropensityFunctions.h");
                        strcpy(tempname_c_str, tempname.c_str());
			model.writeCustomPropensityFunctionsFile(tempname_c_str);
#endif

			//record current path so we can cd back to it after compiling
			std::string currentPath=boost::filesystem::current_path().string();

			std::string stochkit_home_string("$STOCHKIT_HOME");
			std::string stochkit_home_path_string;
                        char* stochkithomePath;
			stochkithomePath = getenv("STOCHKIT_HOME");
			if (stochkithomePath!=NULL){
				stochkit_home_path_string = stochkithomePath;
                                std::size_t found = stochkit_home_path_string.find(' ');
                                if(found!=std::string::npos){
				        boost::filesystem::path current_home(stochkit_home_path_string);
					boost::filesystem::path temp_path = boost::filesystem::unique_path("/tmp/tmp.stochkit.%%%%-%%%%-%%%%-%%%%");
                                        boost::filesystem::create_symlink(current_home, temp_path);
					stochkit_home_string = boost::filesystem::system_complete(temp_path).string();
                                }
			} else {
				std::cerr << "StochKit ERROR (StandardDriverUtilities::compileMixed): $STOCHKIT_HOME is not defined, please define the environment variable or use ssa/tau-leaping script in the StochKit2 home directory.\n";
				std::cerr << "Simulation terminated.\n";
				exit(1);
			}
    
			//updated so generated code path is a full path sanft 2011/03/21
			std::string makeCommand=(std::string)"cd \"" + stochkit_home_string + "/src\"; make "+executableName+" GENERATED_CODE_PATH=\""+commandLine.getGeneratedCodeDir()+"\" STOCHKIT_HOME=\"" +stochkit_home_string +"\" --silent";

			//redirect any errors from make to a log file
			//updated so generated code path is a full path sanft 2011/03/21
			makeCommand+=" > \""+commandLine.getGeneratedCodeDir()+"/.compile-log.txt\" 2>&1";

			std::cout << "StochKit MESSAGE: compiling generated code...this will take a few moments...\n";
			int returnValue=system(makeCommand.c_str());

			if(stochkit_home_string !="$STOCHKIT_HOME"){
				boost::filesystem::path temp_path(stochkit_home_string);
				boost::filesystem::remove(stochkit_home_string);
			}

			if (returnValue!=0) {
				std::cout << "StochKit ERROR: compile of generated code failed.  Simulation terminated.\n";
				//copy hidden compile-log to visible log
				//updated so generated code path is a full path sanft 2011/03/21
				std::string command="cp \""+commandLine.getGeneratedCodeDir()+"/.compile-log.txt\" \""+commandLine.getGeneratedCodeDir()+"/compile-log.txt\"";

				system(command.c_str());

				std::cout << "Check log file \"" << commandLine.getGeneratedCodeDir()<<"/compile-log.txt\" for error messages.\n";
				exit(1);
			}

			//go back to original path
			std::string cd="cd \""+currentPath+"\"";
			system(cd.c_str());
		}
	}
#endif

	std::string StandardDriverUtilities::size_t2string(std::size_t number) {
		std::string theString;
		std::stringstream ss;
		ss<<number;
		theString=ss.str();
		return theString;
	}

}
