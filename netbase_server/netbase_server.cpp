// netbase_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using boost::asio::ip::udp;
using namespace std;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));

	try
	{
		boost::asio::io_service io_service;

		udp::socket socket(io_service, udp::endpoint(udp::v4(), 13999));
		socket.non_blocking(true);

		size_t packed_id = 0;
		for (size_t tick = 0;; ++tick)
		{
			udp::endpoint remote_endpoint;
			vector<unsigned char> buf(1024);
			boost::system::error_code error;
				
			size_t received;
			while ((received = socket.receive_from(boost::asio::buffer(buf), remote_endpoint, 0, error)) > 0)
			{
				if (error && error != boost::asio::error::message_size)
					throw boost::system::system_error(error);

				stringstream ss;
				ss << "packet " << packed_id++;
				string message = ss.str();

				boost::system::error_code ignored_error;
				socket.send_to(boost::asio::buffer(message), remote_endpoint, 0, ignored_error);

				cout << "responding to " << remote_endpoint.address() << ":" << remote_endpoint.port() << endl;
			}

			cout << "tick " << tick << endl;
			boost::this_thread::sleep_for(boost::chrono::seconds(1));
		}

	}
	catch (const boost::system::system_error& e)
	{
		cerr << "[" << e.code() << "] " << e.what() << endl;
	}
	catch (const std::exception& e)
	{
		cerr << "error: " << e.what() << endl;
	}

	return 0;
}

