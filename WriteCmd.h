#ifndef WRITECMD_H
#define WRITECMD_H

#include <boost/thread.hpp>
#include "pgdb.h"
#include "Gateway.h"
using namespace std;
using namespace boost;

class WriteCmd
{ 
	public:

		WriteCmd(boost::asio::io_service& io);
		~WriteCmd();
	//	void  run(boost::asio::io_service& io);
		void  GetCmdFromDB();
		int	  UpdateCmdStatus(int sn, int cmd_status, int reval);
		void  Timer(const boost::system::error_code& ec, timer_pt async_timer);
		void   correctTime( );

	public:
		timer_pt async_timer;

	private:
		GatewayDB db;	
};

#endif