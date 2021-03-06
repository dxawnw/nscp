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

#include "threaded_logger.hpp"


const static std::string QUIT_MESSAGE = "$$QUIT$$";
const static std::string CONFIGURE_MESSAGE = "$$CONFIGURE$$";
const static std::string SET_CONFIG_MESSAGE = "$$SET_CONFIG$$";

namespace nsclient {
	namespace logging {
		namespace impl {
			threaded_logger::threaded_logger(logging_subscriber *subscriber_manager, log_driver_instance background_logger) : subscriber_manager_(subscriber_manager), background_logger_(background_logger) {}
			threaded_logger::~threaded_logger() {
				shutdown();
			}

			void threaded_logger::do_log(const std::string data) {
				push(data);
			}
			void threaded_logger::push(const std::string &data) {
				log_queue_.push(data);
			}

			void threaded_logger::thread_proc() {
				std::string data;
				while (true) {
					try {
						log_queue_.wait_and_pop(data);
						if (data == QUIT_MESSAGE) {
							return;
						} else if (data == CONFIGURE_MESSAGE) {
							if (background_logger_)
								background_logger_->asynch_configure();
						} else if (data.size() > SET_CONFIG_MESSAGE.size() && data.substr(0, SET_CONFIG_MESSAGE.size()) == SET_CONFIG_MESSAGE) {
							background_logger_->set_config(data.substr(SET_CONFIG_MESSAGE.size()));
						} else {
							if (background_logger_->is_console()) {
								std::pair<bool, std::string> m = logger_helper::render_console_message(is_oneline(), data);
								if (!is_no_std_err() && m.first)
									std::cerr << m.second;
								else
									std::cout << m.second;
							}
							if (background_logger_)
								background_logger_->do_log(data);
							subscriber_manager_->on_log_message(data);
						}
					} catch (const std::exception &e) {
						logger_helper::log_fatal(std::string("Failed to process log message: ") + e.what());
					} catch (...) {
						logger_helper::log_fatal("Failed to process log message");
					}
				}
			}

			void threaded_logger::asynch_configure() {
				push(CONFIGURE_MESSAGE);
			}
			void threaded_logger::synch_configure() {
				background_logger_->synch_configure();
			}
			bool threaded_logger::startup() {
				if (is_started())
					return true;
				thread_ = boost::thread(boost::bind(&threaded_logger::thread_proc, this));
				return nsclient::logging::log_driver_interface_impl::startup();
			}
			bool threaded_logger::shutdown() {
				if (!is_started())
					return true;
				try {
					push(QUIT_MESSAGE);
					if (!thread_.timed_join(boost::posix_time::seconds(10))) {
						logger_helper::log_fatal("Failed to exit log slave!");
						nsclient::logging::log_driver_interface_impl::shutdown();
						return false;
					}
					background_logger_->shutdown();
					return nsclient::logging::log_driver_interface_impl::shutdown();
				} catch (const std::exception &e) {
					logger_helper::log_fatal(std::string("Failed to exit log slave: ") + e.what());
				} catch (...) {
					logger_helper::log_fatal("Failed to exit log slave");
				}
				return false;
			}
			void threaded_logger::set_config(const std::string &key) {
				std::string message = SET_CONFIG_MESSAGE + key;
				push(message);
			}
		}
	}
}