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

#pragma once

#include <utils.h>
#include <strEx.h>

#include <socket/client.hpp>

#include <nscapi/nscapi_settings_helper.hpp>
#include <nscapi/nscapi_protobuf_functions.hpp>
#include <nscapi/nscapi_core_helper.hpp>

#include <boost/make_shared.hpp>

namespace nscp_handler {
	namespace sh = nscapi::settings_helper;

	struct nrpe_target_object : public nscapi::targets::target_object {
		typedef nscapi::targets::target_object parent;

		nrpe_target_object(std::string alias, std::string path) : parent(alias, path) {
			set_property_int("timeout", 30);
			set_property_string("certificate", "");
			set_property_string("certificate key", "");
			set_property_string("certificate format", "PEM");
			set_property_string("allowed ciphers", "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
			set_property_string("verify mode", "none");
			set_property_string("password", "");
		}

		nrpe_target_object(const nscapi::settings_objects::object_instance other, std::string alias, std::string path) : parent(other, alias, path) {}

		virtual void read(boost::shared_ptr<nscapi::settings_proxy> proxy, bool oneliner, bool is_sample) {
			parent::read(proxy, oneliner, is_sample);

			nscapi::settings_helper::settings_registry settings(proxy);

			nscapi::settings_helper::path_extension root_path = settings.path(get_path());
			if (is_sample)
				root_path.set_sample();

			root_path.add_key()

				("password", sh::string_fun_key<std::string>(boost::bind(&parent::set_property_string, this, "password", _1)),
					"PASSWORD", "The password to use to authenticate towards the server.")
				;
			settings.register_all();
			settings.notify();
			settings.clear();

			add_ssl_keys(root_path);

			settings.register_all();
			settings.notify();
		}

		virtual void translate(const std::string &key, const std::string &value) {
			parent::translate(key, value);
		}
	};

	struct options_reader_impl : public client::options_reader_interface {
		virtual nscapi::settings_objects::object_instance create(std::string alias, std::string path) {
			return boost::make_shared<nrpe_target_object>(alias, path);
		}
		virtual nscapi::settings_objects::object_instance clone(nscapi::settings_objects::object_instance parent, const std::string alias, const std::string path) {
			return boost::make_shared<nrpe_target_object>(parent, alias, path);
		}

		void process(boost::program_options::options_description &desc, client::destination_container &source, client::destination_container &target) {
			add_ssl_options(desc, target);

			desc.add_options()
				("password,p", po::value<std::string>()->notifier(boost::bind(&client::destination_container::set_string_data, &target, "password", _1)),
					"Password")
				;
		}
	};
}