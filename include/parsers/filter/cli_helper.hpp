/*
 * Copyright 2004-2016 The NSClient++ Authors - https://nsclient.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <parsers/where/engine_impl.hpp>
#include <parsers/filter/modern_filter.hpp>

#include <nscapi/macros.hpp>
#include <nscapi/nscapi_plugin_wrapper.hpp>
#include <nscapi/nscapi_settings_proxy.hpp>
#include <nscapi/nscapi_program_options.hpp>
#include <nscapi/nscapi_protobuf_functions.hpp>
#include <nscapi/nscapi_protobuf.hpp>

namespace modern_filter {
	struct data_container {
		std::vector<std::string> filter_string, warn_string, crit_string, ok_string;
		std::string syntax_empty, syntax_ok, syntax_top, syntax_detail, syntax_perf, perf_config, empty_state, syntax_unique;
		bool debug, escape_html;
		data_container() : debug(false), escape_html(false) {}
	};

	struct perf_writer : public perf_writer_interface {
		Plugin::QueryResponseMessage::Response::Line &line;
		perf_writer(Plugin::QueryResponseMessage::Response::Line &line) : line(line) {}
		virtual void write(const parsers::where::performance_data &data) {
			::Plugin::Common::PerformanceData* perf = line.add_perf();
			perf->set_alias(data.alias);
			if (data.int_value) {
				const parsers::where::performance_data::perf_value<long long> &value = *data.int_value;
				Plugin::Common::PerformanceData::IntValue* perfData = perf->mutable_int_value();
				if (!data.unit.empty())
					perfData->set_unit(data.unit);
				perfData->set_value(value.value);
				if (value.warn)
					perfData->set_warning(*value.warn);
				if (value.crit)
					perfData->set_critical(*value.crit);
				if (value.minimum)
					perfData->set_minimum(*value.minimum);
				if (value.maximum)
					perfData->set_maximum(*value.maximum);
			} else if (data.float_value) {
				const parsers::where::performance_data::perf_value<double> &value = *data.float_value;
				Plugin::Common::PerformanceData::FloatValue* perfData = perf->mutable_float_value();
				if (!data.unit.empty())
					perfData->set_unit(data.unit);
				perfData->set_value(value.value);
				if (value.warn)
					perfData->set_warning(*value.warn);
				if (value.crit)
					perfData->set_critical(*value.crit);
				if (value.minimum)
					perfData->set_minimum(*value.minimum);
				if (value.maximum)
					perfData->set_maximum(*value.maximum);
			} else if (data.string_value) {
				const parsers::where::performance_data::perf_value<std::string> &value = *data.string_value;
				Plugin::Common::PerformanceData::StringValue* perfData = perf->mutable_string_value();
				perfData->set_value(value.value);
			}
		}
	};

	template<class T>
	struct cli_helper : public  boost::noncopyable {
		data_container &data;
		boost::program_options::options_description desc;
		const Plugin::QueryRequestMessage::Request &request;
		Plugin::QueryResponseMessage::Response *response;
		bool show_all;
		std::string syntax_description;

		cli_helper(const Plugin::QueryRequestMessage::Request &request, Plugin::QueryResponseMessage::Response *response, data_container &data)
			: data(data)
			, desc("Allowed options for " + request.command())
			, request(request)
			, response(response)
			, show_all(false) {}

		~cli_helper() {}

		boost::program_options::options_description& get_desc() {
			return desc;
		}

		void set_filter_syntax(const boost::tuple<std::string, std::string> &filter_syntax) {
			syntax_description = "Available options : \n\nKey\tValue\n" + filter_syntax.get<0>() + filter_syntax.get<1>() + "\n\n";

		}
		void add_filter_option(const std::string filter) {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *filter_op = boost::program_options::value<std::vector<std::string> >(&data.filter_string);
			if (!filter.empty()) {
				std::vector<std::string> def;
				def.push_back(filter);
				filter_op->default_value(def, filter);
			}

			desc.add_options()
				("filter", filter_op,
				(std::string("Filter which marks interesting items.\nInteresting items are items which will be included in the check.\nThey do not denote warning or critical state instead it defines which items are relevant and you can remove unwanted items.\n") + syntax_description).c_str())
				;
		}
		void add_warn_option(const std::string warn) {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *warn_op = boost::program_options::value<std::vector<std::string> >(&data.warn_string);
			if (!warn.empty()) {
				std::vector<std::string> def;
				def.push_back(warn);
				warn_op->default_value(def, warn);
			}

			desc.add_options()
				("warning", warn_op,
				(std::string("Filter which marks items which generates a warning state.\nIf anything matches this filter the return status will be escalated to warning.\n") + syntax_description).c_str())
				("warn", boost::program_options::value<std::vector<std::string> >(),
				"Short alias for warning")
				;
		}
		void add_warn_option(const std::string warn1, const std::string warn2) {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *warn_op = boost::program_options::value<std::vector<std::string> >(&data.warn_string);
			std::vector<std::string> def;
			def.push_back(warn1);
			def.push_back(warn2);
			warn_op->default_value(def, warn1 + ", " + warn2);

			desc.add_options()
				("warning", warn_op,
				(std::string("Filter which marks items which generates a warning state.\nIf anything matches this filter the return status will be escalated to warning.\n") + syntax_description).c_str())
					("warn", boost::program_options::value<std::vector<std::string> >(),
						"Short alias for warning")
				;
		}
		void add_crit_option(const std::string crit) {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *crit_op = boost::program_options::value<std::vector<std::string> >(&data.crit_string);
			if (!crit.empty()) {
				std::vector<std::string> def;
				def.push_back(crit);
				crit_op->default_value(def, crit);
			}

			desc.add_options()
				("critical", crit_op,
				(std::string("Filter which marks items which generates a critical state.\nIf anything matches this filter the return status will be escalated to critical.\n") + syntax_description).c_str())
				("crit", boost::program_options::value<std::vector<std::string> >(),
					"Short alias for critical.")
				;
		}
		void add_crit_option(const std::string crit1, const std::string crit2) {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *crit_op = boost::program_options::value<std::vector<std::string> >(&data.crit_string);
			std::vector<std::string> def;
			def.push_back(crit1);
			def.push_back(crit2);
			crit_op->default_value(def, crit1 + ", " + crit2);

			desc.add_options()
				("critical", crit_op,
				(std::string("Filter which marks items which generates a critical state.\nIf anything matches this filter the return status will be escalated to critical.\n") + syntax_description).c_str())
					("crit", boost::program_options::value<std::vector<std::string> >(),
						"Short alias for critical.")
				;
		}
		void add_ok_option(const std::string ok = "") {
			typedef boost::program_options::typed_value<std::vector<std::string> > filter_op_type;
			filter_op_type *ok_op = boost::program_options::value<std::vector<std::string> >(&data.ok_string);
			if (!ok.empty()) {
				std::vector<std::string> def;
				def.push_back(ok);
				ok_op->default_value(def, ok);
			}

			desc.add_options()
				("ok", ok_op,
				(std::string("Filter which marks items which generates an ok state.\nIf anything matches this any previous state for this item will be reset to ok.\n") + syntax_description).c_str())
				;
		}
		void add_misc_options(std::string empty_state = "ignored") {
			boost::program_options::typed_value<std::string> *empty_state_op = boost::program_options::value<std::string>(&data.empty_state);
			boost::program_options::typed_value<std::string> *perf_config_op = boost::program_options::value<std::string>(&data.perf_config);

			if (!empty_state.empty())
				empty_state_op->default_value(empty_state);
			if (!data.perf_config.empty())
				perf_config_op->default_value(data.perf_config);
			desc.add_options()
				("debug", boost::program_options::bool_switch(&data.debug),
					"Show debugging information in the log")
				("show-all", boost::program_options::bool_switch(&show_all),
					"Show details for all matches regardless of status (normally details are only showed for warnings and criticals).")
				("empty-state", empty_state_op,
					"Return status to use when nothing matched filter.\nIf no filter is specified this will never happen unless the file is empty.")
				("perf-config", perf_config_op,
					"Performance data generation configuration\nTODO: obj ( key: value; key: value) obj (key:valuer;key:value)")
				("escape-html", boost::program_options::bool_switch(&data.escape_html),
					"Escape any < and > characters to prevent HTML encoding")
				;
			nscapi::program_options::add_help(desc);
		}
		void add_options(std::string warn, std::string crit, std::string filter, const boost::tuple<std::string,std::string> &filter_syntax, std::string empty_state = "ignored") {
			set_filter_syntax(filter_syntax);
			add_filter_option(filter);
			add_warn_option(warn);
			add_crit_option(crit);
			add_ok_option();
			add_misc_options(empty_state);
		}
		void add_options(const boost::tuple<std::string, std::string> &filter_syntax, std::string empty_state = "ignored") {
			set_filter_syntax(filter_syntax);
			add_ok_option();
			add_misc_options(empty_state);
		}

		void parse_options_post(boost::program_options::variables_map &vm) {
			if (show_all) {
				if (data.syntax_top.find("${problem_list}") != std::string::npos)
					boost::replace_all(data.syntax_top, "${problem_list}", "${detail_list}");
				else
					data.syntax_top = "${detail_list}";
			}
			if (boost::contains(data.syntax_top, "detail_list")
				|| boost::contains(data.syntax_top, "(list)")
				|| boost::contains(data.syntax_top, "{list}")
				|| boost::contains(data.syntax_top, "match_list")
				|| boost::contains(data.syntax_top, "lines")
				)
				data.syntax_ok = "";
			if (vm.count("warn"))
				data.warn_string = vm["warn"].as<std::vector<std::string> >();
			if (vm.count("crit"))
				data.crit_string = vm["crit"].as<std::vector<std::string> >();
		}

		bool parse_options(boost::program_options::positional_options_description p) {
			boost::program_options::variables_map vm;
			if (!nscapi::program_options::process_arguments_from_request(vm, desc, request, *response, p))
				return false;
			parse_options_post(vm);
			return true;
		}
		bool parse_options() {
			boost::program_options::variables_map vm;
			if (!nscapi::program_options::process_arguments_from_request(vm, desc, request, *response))
				return false;
			parse_options_post(vm);
			return true;
		}
		bool parse_options(std::vector<std::string> &extra) {
			boost::program_options::variables_map vm;
			if (!nscapi::program_options::process_arguments_from_request(vm, desc, request, *response, true, extra))
				return false;
			parse_options_post(vm);
			return true;
		}

		bool empty() const {
			return data.warn_string.empty() && data.crit_string.empty();
		}
		void append_all_filters(const std::string verb, const std::string filter) {
			if (data.filter_string.empty()) {
				data.filter_string.push_back(filter);
			}
			BOOST_FOREACH(std::string &f, data.filter_string) {
				f = "(" + f + ")" + verb + " " + filter;
			}
		}
		void set_default_index(const std::string filter) {
			if (data.syntax_unique.empty())
				data.syntax_unique = filter;
		}
		bool build_filter(T &filter) {
			std::string tmp_msg;
			data.filter_string.erase(std::remove(data.filter_string.begin(), data.filter_string.end(), "none"), data.filter_string.end());
			data.ok_string.erase(std::remove(data.ok_string.begin(), data.ok_string.end(), "none"), data.ok_string.end());
			data.warn_string.erase(std::remove(data.warn_string.begin(), data.warn_string.end(), "none"), data.warn_string.end());
			data.crit_string.erase(std::remove(data.crit_string.begin(), data.crit_string.end(), "none"), data.crit_string.end());

			if (!filter.build_syntax(data.syntax_top, data.syntax_detail, data.syntax_perf, data.perf_config, data.syntax_ok, data.syntax_empty, tmp_msg)) {
				nscapi::protobuf::functions::set_response_bad(*response, tmp_msg);
				return false;
			}
			if (!data.syntax_unique.empty()) {
				if (!filter.build_index(data.syntax_unique, tmp_msg)) {
					nscapi::protobuf::functions::set_response_bad(*response, tmp_msg);
					return false;
				}
			}
			if (!filter.build_engines(data.debug, data.filter_string, data.ok_string, data.warn_string, data.crit_string)) {
				nscapi::protobuf::functions::set_response_bad(*response, "Failed to build engines");
				return false;
			}

			std::string error;
			if (!filter.validate(error)) {
				nscapi::protobuf::functions::set_response_bad(*response, "Failed to validate filter see log for details: " + error);
				return false;
			}

			filter.start_match();
			return true;
		}
		void set_default_perf_config(const std::string conf) {
			data.perf_config = conf;
		}
		void add_syntax(const std::string &default_top_syntax, const boost::tuple<std::string, std::string> &syntax, const std::string &default_detail_syntax, const std::string &default_perf_syntax, const std::string &default_empty_syntax, const std::string &default_ok_syntax) {
			std::string tk = "Top level syntax.\n"
				"Used to format the message to return can include text as well as special keywords which will include information from the checks.\n"
				"To add a keyword to the message you can use two syntaxes either ${keyword} or %(keyword) (there is no difference between them apart from ${} can be difficult to excpae on linux).\n"
				"The available keywords are: \n\nKey\tValue\n" + syntax.get<0>() + "\n";
			std::string dk = "Detail level syntax.\n"
				"Used to format each resulting item in the message.\n"
				"%(list) will be replaced with all the items formated by this syntax string in the top-syntax.\n"
				"To add a keyword to the message you can use two syntaxes either ${keyword} or %(keyword) (there is no difference between them apart from ${} can be difficult to excpae on linux).\n"
				"The available keywords are: \n\nKey\tValue\n" + syntax.get<1>() + "\n";
			std::string pk = "Performance alias syntax.\n"
				"This is the syntax for the base names of the performance data.\n"
				"Possible values are: \n\nKey\tValue\n" + syntax.get<1>() + "\n";
			std::string ek = "Empty syntax.\n"
				"DEPRECATED! This is the syntax for when nothing matches the filter.\n"
				"Possible values are: \n\nKey\tValue\n" + syntax.get<0>() + "\n";
			std::string ok = "ok syntax.\n"
				"DEPRECATED! This is the syntax for when an ok result is returned.\n"
				"This value will not be used if your syntax contains %(list) or %(count).";

			desc.add_options()
				("top-syntax", boost::program_options::value<std::string>(&data.syntax_top)->default_value(default_top_syntax), tk.c_str())
				("ok-syntax", boost::program_options::value<std::string>(&data.syntax_ok)->default_value(default_ok_syntax), ok.c_str())
				("empty-syntax", boost::program_options::value<std::string>(&data.syntax_empty)->default_value(default_empty_syntax), ek.c_str())
				("detail-syntax", boost::program_options::value<std::string>(&data.syntax_detail)->default_value(default_detail_syntax), dk.c_str())
				("perf-syntax", boost::program_options::value<std::string>(&data.syntax_perf)->default_value(default_perf_syntax), pk.c_str())
				;
		}

		void add_index(const boost::tuple<std::string, std::string> &syntax, const std::string &default_unique_syntax) {
			std::string tk = "Unique syntax.\n"
				"Used to filter unique items (counted will still increase but messages will not repeaters: \n\nKey\tValue\n" + syntax.get<1>() + "\n";

			desc.add_options()
				("unique-index", boost::program_options::value<std::string>(&data.syntax_unique)->default_value(default_unique_syntax), tk.c_str())
				;
		}

		void post_process(T &filter) {
			filter.match_post();
			Plugin::QueryResponseMessage::Response::Line *line = response->add_lines();
			modern_filter::perf_writer writer(*line);
			std::string msg = filter.get_message();
			if (data.escape_html) {
				boost::replace_all(msg, "<", "&lt;");
				boost::replace_all(msg, ">", "&gt;");
			}
			line->set_message(msg);
			filter.fetch_perf(&writer);
			if ((data.empty_state != "ignored") && (!filter.summary.has_matched()))
				response->set_result(nscapi::protobuf::functions::nagios_status_to_gpb(nscapi::plugin_helper::translateReturn(data.empty_state)));
			else
				response->set_result(nscapi::protobuf::functions::nagios_status_to_gpb(filter.summary.returnCode));
		}
	};
}