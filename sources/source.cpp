// Copyright 2018 Your Name <your_email>

#include <header.hpp>
#include <iostream>
#include<boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <system_error>
#include <unistd.h>

static const int buf_size =500;
static boost::thread_group threads;
using namespace boost::asio;
static io_service service;
static ip::tcp::acceptor acceptor{service, ip::tcp::endpoint{ip::tcp::v4(), 8001}};
typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
static std::vector<socket_ptr> sockets;
static boost::mutex mutex1,mutex_socket,mutex_clients;
static boost::recursive_mutex mutex;
static std::vector<int> num_close;
void clear_buf(char * buf,int size)
{
    for (int i=0;i<size;++i)
    {
        buf[i]='/0';
    }
}

struct client
{
    ip::tcp::socket _sock{service};
    bool _status=true;
    void write (std:: string msg)
    {
        _sock.write_some(boost::asio::buffer(msg,7));
    }
    std::string _reply;
    void read_str()
    {
        if (_sock.available())
        {
            char buf[200] ;
            int bytes=_sock.read_some(boost::asio::buffer(buf,200));
            for (int i=0;i<bytes;++i)
            {
                _reply[i]=std::move(buf[i]);
            }
        }

    }
    void ping()
    {
        write("ping_ok");
    }
    void communicate()
    {
        try
        {
            ping();
            read_str();
            std::cout<< _reply;
        }
        catch ( boost::system::system_error&)
        {
            if (_sock.available())
            {
                _sock.close();
            }
            _status=false;
            std::cout<<"no client";

        }


    }
    bool disconnected()
    {
        return !_status;
    }

    ~client(){_sock.close();}
};


typedef boost::shared_ptr<client> client_ptr;
static std::vector<client_ptr> clients;
boost::recursive_mutex cs;


void communication_with_server()
{
    std::vector<client>::iterator it;
    while (true)
    {
        boost::this_thread::sleep( boost::posix_time::millisec(1));
        boost::recursive_mutex::scoped_lock lk(cs);

        for (auto &a: clients)
        {
            a->communicate();
        }
        for (auto i=clients.begin();i!=clients.end();)
        {
            if (!(i->get()->_status))
            {
                i=clients.erase(i);
            }
            else{
                ++i;
            }
        }
    }
}

void access_func()
{

    while(true)
    {
        client_ptr one(new client);
        acceptor.accept(one->_sock);
        boost::recursive_mutex::scoped_lock lk(cs);
        clients.push_back(one);
    }
}

int main(int argc, char* argv[])
{

    boost::thread first(access_func);
    boost::thread second(communication_with_server);
    first.join();
    second.join();
    return 0;
}
