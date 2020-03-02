// Copyright 2018 Your Name <your_email>

#include <header.hpp>
#include <iostream>
#include<boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <system_error>
#include <unistd.h>

static const int buf_size =500;
static boost::thread_group threads;
using namespace boost::asio;
static io_service service;
typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
static std::vector<socket_ptr> sockets;
static boost::mutex mutex,mutex1,mutex_socket,mutex_clients;
ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(),8001));
void clear_buf(char * buf,int size)
{
    for (int i=0;i<size;++i)
    {
        buf[i]='/0';
    }
}

struct client
{
    socket_ptr _sock ;
    std::string _name;
    boost::thread::id pid=boost::this_thread::get_id();
    std::string _received_message;
    bool _status = true ;
    client(){}
    client(const std::string& name,socket_ptr &&sock):_name(name),
    _sock(std::move(sock))
    {
        for (int i=0;i<5;++i)
        {
            send_message(_name);
            receive_message();
            send_message("krut\n");
            _name=_received_message;
            std::cout<<_received_message<<std::endl;
        }
    }


    int send_message(std::string msg)
    {
        int length=msg.size();
        _sock->write_some(boost::asio::buffer(&length,4));
        _sock->write_some(boost::asio::buffer(msg,msg.size()));
    }

    int receive_message()
    {
        char buff [buf_size];
        int length=0;
        clear_buf(buff,buf_size);
        _sock->read_some(buffer(&length,4));
        int count =_sock->read_some(buffer(buff,length));
        std::string tmp(buff,buf_size);
        _received_message=tmp.substr(0,tmp.find('/0'));
        return count;
    }
    bool is_connected()
    {
        return _status;
    }
    void client_out()
    {
        _sock->close();
        _name.pop_back();
        _status=false;
        std::cout<<std::endl<<"client :"+_name<<" is out "<<boost::this_thread::get_id()<<std::endl;
    }
    void send_list()
    {
    }
    ~client()
    {
        client_out();
    }
};

static std::vector<client> clients;

int check_user(socket_ptr& sock,std::string& str)
{
    char buff [buf_size];
    int count_of_bytes=0;
    clear_buf(buff,buf_size);
    sock->read_some(boost::asio::buffer(&count_of_bytes,4));
    sock->read_some(boost::asio::buffer(buff,count_of_bytes));
    std::string msg(buff,count_of_bytes);
    str=msg;
    if(msg.find('\n') == std::string::npos)
    {
        msg="incorrect format of letter add \\n";
        count_of_bytes=msg.size();
        sock->write_some(boost::asio::buffer(&count_of_bytes,4));
        sock->write_some(boost::asio::buffer(msg,count_of_bytes));
        return 1;
    }
    else
    {
        msg="correct format\n";
        count_of_bytes=msg.size();
        sock->write_some(boost::asio::buffer(&count_of_bytes,4));
        sock->write_some(boost::asio::buffer(msg,count_of_bytes));
        return 0;
    }
}


void communication_with_server()
{

         while (!clients.size())
         {

         }

         while(true)
         {
             for (auto &a : clients)
             {
                 if (a.is_connected()){}
                     else{}
             }
         }

}

void access_func()
{
    while(true)
    {
        socket_ptr sock(new ip::tcp::socket(service));
        acceptor.accept(*sock);
        mutex_clients.lock();
        clients.push_back(client("padavan",std::move(sock)));
        mutex_clients.unlock();
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
