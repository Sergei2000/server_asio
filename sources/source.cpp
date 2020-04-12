// Copyright 2018 Your Name <your_email>

#include <header.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <system_error>

//using namespace boost::asio;
static boost::asio::io_service service;
static boost::asio::ip::tcp::acceptor acceptor
        {service, boost::asio::ip::tcp::endpoint
        {boost::asio::ip::tcp::v4(), 8001}};
boost::recursive_mutex cs, ds;
static std::vector<std::string> client_list;
struct client {
    boost::asio::ip::tcp::socket _sock
            {service};
    boost::posix_time::ptime now;
    bool _status = true;
    bool _initial = true;
    uint64_t _clients_num = 0;
    std::string _name, _request;

    void read_request() {
        boost::asio::streambuf buffer{};
        read_until(_sock, buffer, "\n");
        std::string request((std::istreambuf_iterator<char>(&buffer)),
        std::istreambuf_iterator<char>());
        _request = request;
    }

    void write_reply(std::string reply) {
        reply += "\r\n";
        _sock.write_some(boost::asio::buffer(reply));
    }

    void reply_name() {
        read_request();
        _name = _request;
        write_reply(_name);
        BOOST_LOG_TRIVIAL(info) << _name;
        boost::recursive_mutex::scoped_lock lk(ds);
        client_list.push_back(_name);
    }

    void ping() {
        write_reply("ping_ok\n");
    }

    void analyse_request() {
        read_request();
        if (_request == "ping\n\r\n") {
            boost::recursive_mutex::scoped_lock lk(ds);
            if (_clients_num != client_list.size()) {
                write_reply("changed\n");
                _clients_num = client_list.size();
            } else {
                ping();
            }
        } else {
            if (_request == "list\n\r\n") {
                std::string list;
                boost::recursive_mutex::scoped_lock lk(ds);
                for (std::size_t i = 0; i < client_list.size(); ++i) {
                    list += client_list[i];
                }
                write_reply(list);
            } else {
                ping();
            }
        }
    }

    void communicate() {
        try {
            if (_initial) {
                reply_name();
                _initial = false;
            }
            now = boost::posix_time::microsec_clock::local_time();
            analyse_request();
             boost::posix_time::ptime finish;
            finish = boost::posix_time::microsec_clock::local_time();
            if ((finish-now).total_milliseconds() > 20){
                throw -1;
            }
        }
        catch (...) {
            _status = false;
            std::cout << "socket otkinulsya" << std::endl;
            _sock.close();
            boost::recursive_mutex::scoped_lock lk(ds);
            for (auto i = client_list.begin(); i != client_list.end();) {
                if (i->data() == _name) {
                    i = client_list.erase(i);
                } else {
                    ++i;
                }
            }
            BOOST_LOG_TRIVIAL(error) << _name;
        }
    }
};

typedef boost::shared_ptr<client> client_ptr;
static std::vector<client_ptr> clients;

void access_func() {
    fflush(stdout);
    while (true) {
        client_ptr one(new client);
        acceptor.accept(one->_sock);
        boost::recursive_mutex::scoped_lock lk(cs);
        clients.push_back(one);
        //std::cout<<"zakinul"<<std::endl;
    }
}

void communication_with_server() {
    while (true) {
        boost::this_thread::sleep(boost::posix_time::millisec(1));
        boost::recursive_mutex::scoped_lock lk(cs);
        for (auto &a : clients) {
            a->communicate();
        }
        for (auto i = clients.begin(); i != clients.end(); ) {
            if (!(i->get()->_status)) {
                i = clients.erase(i);
            } else {
                ++i;
            }
        }
    }
}

int main() {
    boost::thread first(access_func);
    boost::thread second(communication_with_server);
    second.join();
    first.join();
    return 0;
}
