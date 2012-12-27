// netbase_client.cpp : Defines the entry point for the console application.
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

		udp::resolver resolver(io_service);
		udp::resolver::query query(udp::v4(), "localhost", "13999");
		udp::endpoint receiver_endpoint = *resolver.resolve(query);

		cout << "resolved server as " << receiver_endpoint.address() << " : " << receiver_endpoint.port() << endl;

		udp::socket socket(io_service);
		socket.open(udp::v4());
		socket.non_blocking(true);

		for (size_t tick = 0; tick < 25; ++tick)
		{
			boost::array<char, 1> send_buf  = {{ 0 }};
			boost::system::error_code error;
			socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);

			cout << "tick" << endl;

			boost::array<char, 1024> recv_buf;
			udp::endpoint sender_endpoint;

			size_t len;
			while ((len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint, 0, error)) > 0)
			{
				if (error && error != boost::asio::error::message_size)
					throw boost::system::system_error(error);

				cout << "received message: ";
				cout.write(recv_buf.data(), len);
				cout << endl;
			}

			boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
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

