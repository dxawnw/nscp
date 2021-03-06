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

#include <boost/date_time.hpp>

class simple_timer {
	boost::posix_time::ptime start_time;
	std::string text;
	bool log;
public:
	simple_timer() {
		start();
	}
	simple_timer(std::string text, bool log) : text(text), log(log) {
		start();
	}
	~simple_timer() {
		if (log)
			std::cout << text << stop() << std::endl;;
	}

	void start() {
		start_time = get();
	}
	unsigned long long stop() {
		boost::posix_time::time_duration diff = get() - start_time;
		start();
		return diff.total_seconds();
	}

private:
	boost::posix_time::ptime get() {
		return boost::posix_time::microsec_clock::local_time();
	}

};