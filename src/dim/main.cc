/*
 * Created by David Lapsley on Mon May 30 2011.
 *
 * Copyright 2003, 2011 MIT Haystack Observatory 
 *  
 * This file is part of mark6.
 *
 * mark6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mark6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mark6.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */


// C includes.
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/resource.h>

// C++ includes.
#include <iostream>
#include <iomanip>
#include <string>
#include <bitset>
#include <sstream>
#include <list>

// Framework includes.
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

// Local includes.
#include <Mark6.h>
#include <SocketBuffer.h>
#include <UDPSocket.h>
#include <IO.h>

// Namespaces.
namespace po = boost::program_options;

// Run simple tests.
void run_tests(std::string src_ip, int src_port) {
  LOG4CXX_INFO(logger, "Running tests.");
  SocketBuffer b;
  UDPSocket s;
  std::string address("0.0.0.0");
  s.bind(address, 8042);
  while (true) {
    stringstream ss;
    int n = s.recvfrom(src_ip, src_port, b);
    ss << "Received " << n << " bytes.";
    LOG4CXX_INFO(logger, ss.str());
  }
  s.close();
}

void build_select_list(fd_set& fdset, int& max_fd, int* fds, int fd_len) {
  FD_ZERO(&fdset);
  for (int i=0; i<fd_len; ++i) {
    if (fds[i] > max_fd)
      max_fd = fds[i];
    FD_SET(fds[i], &fdset);
  }
}

struct WriteThread {
	void operator() (int fd) {
		std::cout << "WriteThread: " << fd << std::endl;
	}
};

void test_hd(const int write_block_size, const int write_mbytes) {
	// Priority.
	std::cout << "Priority: " << getpriority(PRIO_PROCESS, 0) << std::endl;
	// setpriority(PRIO_PROCESS, 0, -1);
	std::cout << "Priority: " << getpriority(PRIO_PROCESS, 0) << std::endl;

  // Initialize file descriptors.
  const int N = 32;
  const int BUF_SIZE = write_block_size;
  char *buf = new char[BUF_SIZE];
  int fds[N];

  for (int i=0; i<N; ++i) {
		if (i==3)
			continue;

    stringstream ss;
    ss << "/mnt/disk" << i << "/scan" << ".m6";
    fds[i] = open(ss.str().c_str(), O_WRONLY | O_CREAT | O_NONBLOCK,
								S_IRWXU);
    if (fds[i] < 0) {
      std::cerr << "Unable to open: " << ss.str() << std::endl;
    }
  }

  long mbytes_left = write_mbytes;
	long mbyte_fraction = 0;

  fd_set fdset;
  int max_fd;
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int ready_fds = 0;

	// Write data to disk.
	Timer t;
	t.start();
	long bytes_written = 0;
  while (mbytes_left > 0) {
    max_fd = 0;
    build_select_list(fdset, max_fd, fds, N);
    ready_fds = select(max_fd + 1, (fd_set*)0, &fdset, (fd_set*)0, &timeout);

    for (int j=0; j<N; ++j) {
			if (j==3)
				continue;
      if (FD_ISSET(fds[j], &fdset)) {
				int nb = writen(fds[j], buf, BUF_SIZE);
				bytes_written += nb;
				mbyte_fraction += nb;
				if (mbyte_fraction > 1000000) {
					--mbytes_left;
					mbyte_fraction = 0;
				}
      }
    }
  }
	t.stop();
	double duration = t.duration();
	std::cout << "Duration:   " << duration << std::endl;
	std::cout << "MBytes:     " << bytes_written/1000000 << std::endl;
	std::cout << "Rate(Mbps): " << 8*bytes_written/(duration*1000000) << std::endl;
	std::cout
		<< "STATS "
		<< write_mbytes << " " 
		<< duration << " " 
		<< write_block_size << " " 
		<< bytes_written*8/(duration*1000000.0) 
		<< std::endl;
}

// Print usage message.
// @param desc Options description message.
// @return None.
void
usage(const po::options_description& desc) {
  cout
    << "mark6 [options]" << endl
    << desc;
}

// Global logger definition.
LoggerPtr logger(Logger::getLogger("mark6"));

// Program entry point.
int
main (int argc, char* argv[])
{
  // Variables to store options.
  string log_config; 
  int port = 0;
  string data_file;
  string hash_type;
  string config;
  string schema_config;
  string cdr_select_string;
  int topx_size = 0;
  int window_size = 0;
  int reorder_window = 0;
  int rt_flag = 0;

  string src_ip;
  int src_port;
	int write_block_size, write_mbytes;

  // Declare supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("v", "print version message")
    ("run-tests", "Run test programs")
    ("test-hd", "Test HD performance")
    (
     "data-file",
     po::value<string>(&data_file)->default_value(string("mark6.dat")),
     "Output data file name"
     )
    (
     "config",
     po::value<string>(&config)->default_value(string("mark6.xml")),
     "XML configuration file name"
     )
    (
     "schema-config",
     po::value<string>(&schema_config)
     ->default_value(string("mark6-schema.cfg")),
     "schema configuration file name"
     )
    (
     "log-config",
     po::value<string>(&log_config)
     ->default_value(string("mark6-log.cfg")),
     "Log configuration file name"
     )
    (
     "hash-type",
     po::value<string>(&hash_type)->default_value(string("static")),
     "Hash type to use (static | dynamic)"
     )
    (
     "port",
     po::value<int>(&port)->default_value(10000),
     "Listening port"
     )
    (
     "topx-size",
     po::value<int>(&topx_size)->default_value(10),
     "Size of topx list"
     )
    (
     "window-size",
     po::value<int>(&window_size)->default_value(900),
     "Size of accumulation window(s)"
     )
    (
     "reorder-window",
     po::value<int>(&reorder_window)->default_value(3600),
     "Size of reordering buffer(s)"
     )
    (
     "rt-flag",
     po::value<int>(&rt_flag)->default_value(0),
     "Enable real time processing(1) or batch processing (0)"
     )
    (
     "cdr-select",
     po::value<string>(&cdr_select_string)
     ->default_value(string("SELECT * FROM cdrs;")),
     "CDR select string."
     )

    (
     "src-ip",
     po::value<string>(&src_ip)
     ->default_value(string("127.0.0.1")),
     "Source IP."
     )
    (
     "src-port",
     po::value<int>(&src_port)->default_value(5000),
     "Source port (5000)"
    )
    (
     "write-mbytes",
     po::value<int>(&write_mbytes)->default_value(1024),
     "Write MBytes (1024)"
    )
    (
     "write-block-size",
     po::value<int>(&write_block_size)->default_value(1024),
     "Write block size (1024)"
    )
    ;

  // Parse options.
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);;
  po::notify(vm);	

  // Configure log subsystem.
  PropertyConfigurator::configure(log_config);

  // Check various options.
  if (vm.count("help")) {
    usage(desc);
    return 1;
  }

  if (vm.count("v")) {
    cout << "Mark6 version: 0.1.0"
         << endl;
    return 1;
  }

  if (vm.count("run-tests")) {
    run_tests(src_ip, src_port);
    return 0;
  }

  if (vm.count("test-hd")) {
    test_hd(write_block_size, write_mbytes);
    return 0;
  }

  if (!vm.count("port") || !vm.count("data-file")
      || !vm.count("config") ) {
    usage(desc);
    return 1;
  }

  // Start processing.
  try {
    LOG4CXX_INFO(logger, "Creating mark6 manager.");

    // Create the server.
    LOG4CXX_INFO(logger, "Creating tcp server.");
    // TCPServer server(IO_SERVICE, port, BLOOM_MANAGER);

    // Initialize the timer.
    if (rt_flag) {
      LOG4CXX_INFO(logger, "Initializing timers.");
      // PROC_TIMER.expires_from_now(boost::posix_time::seconds(PROC_INTERVAL));
      // PROC_TIMER.async_wait(proc_cb);
    } else {
      LOG4CXX_INFO(logger, "Operating in batch mode.");
    }

    // Fire up the reactor.
    LOG4CXX_INFO(logger, "Starting reactor.");
    // IO_SERVICE.run();
  } catch (std::exception& e) {
    cerr << e.what() << endl;
  }

  return 0;
}


