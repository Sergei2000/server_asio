// Copyright 2018 Your Name <your_email>

#include <header.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <system_error>


static const int buf_size = 500;

static boost::thread_group threads;
static io_service service;
static ip::tcp::acceptor acceptor{service, ip::tcp::endpoint{ip::tcp::v4(), 8001}};
typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
static std::vector<socket_ptr> sockets;
static boost::mutex mutex1,mutex_socket,mutex_clients;
static boost::recursive_mutex mutex;
static std::vector<int> num_close;
static std::string client_list="client list: ";
boost::recursive_mutex cs,ds;


struct client
{
    ip::tcp::socket _sock{service};
    bool _status=true;
    bool _initial = true;
    char buf[200] ;
    void clear_buf(char * buf,int size)
    {
        for (int i=0;i<size;++i)
        {
            buf[i]='/0';
        }
    }
    void write (std::string msg)
    {
        int a=msg.size();
       _sock.write_some(boost::asio::buffer(&a,4));
        _sock.write_some(boost::asio::buffer(msg,a));
    }
    std::string _reply,_name;
    void read_str()
    {
        if (_sock.available())
        {
            int counter;
            clear_buf(buf,40);
            _sock.read_some(boost::asio::buffer(&counter,4));
            _sock.read_some(boost::asio::buffer(buf,counter));
            _reply=std::string(buf);
            _reply=_reply.substr(0,_reply.find('/0'));
            //std::cout<< _reply;
            clear_buf(buf,40);
        }

    }
    void ping()
    {
        write("ping_ok\n");
    }
    void communicate()
    {
        try
        {
            alarm(10);
            if (_initial) {
                read_str();
                boost::recursive_mutex::scoped_lock lk(ds);
                _name = _reply;
                if (_name.find("ping\n") != std::string::npos)
                {
                    _name=_name.substr(0,_name.find("ping\n"));
                }
                if (_name.find("client list\n") != std::string::npos)
                {
                    _name=_name.substr(0,_name.find("client list\n"));
                }
                if (client_list.find(_name)==std::string::npos)
                {
                    client_list+=(" "+_name+" ");
                }
                BOOST_LOG_TRIVIAL(info) <<_name;
                _initial=false;
            }
            clear_buf(buf,40);
            read_str();
           // std::cout<<_reply;
            if (_reply=="ping\n")
            {
                ping();
            } else
            {
                if (_reply.find("client list\n")!=std::string::npos)
                {
                    //std::cout<<_reply<<std::endl;
                    boost::recursive_mutex::scoped_lock lk(ds);
                    write(client_list);
                }

            }
          alarm(0);
        }
        catch ( ... )
        {
            if (_sock.available())
            {
                _sock.close();
            }
            _status=false;
            fflush(stdout);
            //std::cout<<"no client"+_name;
            client_list.erase(client_list.find(_name),_name.size());
            BOOST_LOG_TRIVIAL(error) <<_name;
            alarm(0);
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
