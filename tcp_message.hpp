#ifndef TCP_MESSAGE_HPP
#define TCP_MESSAGE_HPP
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/checked_delete.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "Gateway.h"
class Gateway;
#define MAX_BUF_SIZE	300
class tcp_message : boost::noncopyable	//不允许拷贝
{

public:
	typedef boost::function<void(tcp_message*)> destory_type;	//销毁器
	typedef boost::shared_ptr<Gateway> tcp_session_ptr;			//

public:
	unsigned char data[MAX_BUF_SIZE];
	int size;
	char gateway_id[50];	//每个网关的唯一标识

	template<typename Func>
	tcp_message(const tcp_session_ptr& s,Func func)
		:m_session(s),m_destory(func)
	{cout << __FILE__<< "	" << __LINE__  << "	"<<endl;}


	tcp_message(const tcp_session_ptr& s)
		//:m_session(s)
	{}

	
	template<typename Func>
	tcp_message(Func func)
		:m_destory(func)
	{}


	tcp_session_ptr get_session(){
			return m_session;
	}

	void destory()
	{
		if(m_destory) 
			m_destory(this);
		else 
			boost::checked_delete(this);
	}

private:
	tcp_session_ptr m_session;		//tcp连接的shared_ptr
	destory_type m_destory;			//用于自行销毁
};
#endif