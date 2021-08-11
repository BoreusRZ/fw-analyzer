#include "args.hpp"
#include "util.hpp"
#include <argparse/argparse.hpp>
#include <filesystem>
#include "log.hpp"
#include <omp.h>
namespace args{
	void parse(int argc, char** argv){
		argparse::ArgumentParser argparser("analyzer");
		constexpr auto VERBOSE_ARG = "--verbose";
		constexpr auto ANALYZE_ARG = "--analyze";
		constexpr auto PROGRESS_ARG = "--progress";
		constexpr auto RULESET_ARG = "ruleset";
		constexpr auto IPSET_ARG = "--ipset";
		constexpr auto CONFIG_ARG = "--config";
		constexpr auto THREADS_ARG = "--threads";
		argparser.add_argument("-V", VERBOSE_ARG)
			.default_value(false)
			.implicit_value(true)
			.help("enables verbose output");
		argparser.add_argument("-a",ANALYZE_ARG)
			.default_value(std::vector<std::string>{"all"})
			.help("specify wich analysis should be performed");
		argparser.add_argument("-p",PROGRESS_ARG)
			.default_value(false)
			.implicit_value(true)
			.help("shows progress output");
		argparser.add_argument("-t",THREADS_ARG)
			.default_value("1")
			.help("how many cores can be used to calculate");
		argparser.add_argument(CONFIG_ARG)
			.default_value(std::string{""})
			.help("specify path of the config file");
		argparser.add_argument(RULESET_ARG)
			.help("specifys the iptables-save dump path, which contains all rules");
		argparser.add_argument(IPSET_ARG)
			.help("specifys the ipset path, which contains ipset configuration");

		try {
			argparser.parse_args(argc, argv);
		}
		catch (std::runtime_error& err) {
			std::cout << err.what() << std::endl;
			std::cout << argparser;
			exit(0);
		}
#define PRESENT(var,arg_name) var = argparser.present<decltype(var)::value_type>(arg_name)
#define GET(var,arg_name) var = argparser.get<decltype(var)>(arg_name)
#define USED(var,arg_name) var = argparser.is_used(arg_name)
		PRESENT(ruleset_filename,RULESET_ARG);
		PRESENT(ipset_filename,IPSET_ARG);
		GET(verbose,VERBOSE_ARG);
		GET(progress,PROGRESS_ARG);
		std::vector<std::string> analyze;
		GET(analyze,ANALYZE_ARG);
		std::string config_filename;
		GET(config_filename,CONFIG_ARG);
		if(!config_filename.empty())config = config_t(config_filename);
		else{
			constexpr auto default_config = "config.yaml";
			if(std::filesystem::exists(default_config)){
				config = config_t(default_config);
				std::cout << "using config:" << std::endl;
				config.print();
			}
		}
		/* threads = stoi(argparser.get<std::string>(THREADS_ARG)); */
		/* omp_set_num_threads(threads); */
		/* mlog::info("setting {} threads\n",threads); */
#undef PRESENT
#undef GET
#undef USED
		for(const auto& idk : analyze){
			for(auto toAnalyze : util::split(idk,",")){
				if(toAnalyze == "all"){
				}else if(toAnalyze == "consumers"){
					analyze_consumers = true;
				}
			}
		}
	}
	
}
