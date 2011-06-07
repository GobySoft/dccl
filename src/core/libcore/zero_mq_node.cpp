// copyright 2010-2011 t. schneider tes@mit.edu
//
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include <boost/asio/detail/socket_ops.hpp> // for network_to_host_long

#include "goby/util/binary.h" // for hex_encode
#include "goby/util/logger.h" // for glog & manipulators die, warn, group(), etc.
#include "goby/util/string.h" // for goby::util::as

#include "zero_mq_node.h"
#include "exception.h"

using goby::util::as;
using goby::glog;
using goby::util::hex_encode;



goby::core::ZeroMQNode::ZeroMQNode()
    : context_(1)
{
}

void goby::core::ZeroMQNode::process_cfg(const protobuf::ZeroMQNodeConfig& cfg)
{
    for(int i = 0, n = cfg.socket_size(); i < n; ++i)
    {
        if(!sockets_.count(cfg.socket(i).socket_id()))
        {
            // TODO (tes) - check for compatible socket type
            boost::shared_ptr<zmq::socket_t> new_socket(
                new zmq::socket_t(context_, socket_type(cfg.socket(i).socket_type())));
            
            sockets_.insert(std::make_pair(cfg.socket(i).socket_id(), new_socket));
            
            //  Initialize poll set
            zmq::pollitem_t item = { *new_socket, 0, ZMQ_POLLIN, 0 };

            // publish sockets can't receive
            if(cfg.socket(i).socket_type() != protobuf::ZeroMQNodeConfig::Socket::PUBLISH)
            {
                register_poll_item(item,
                                   boost::bind(&goby::core::ZeroMQNode::handle_receive,
                                               this, _1, _2, _3, cfg.socket(i).socket_id()));
            }
        }

        boost::shared_ptr<zmq::socket_t> this_socket = socket_from_id(cfg.socket(i).socket_id());
        
        if(cfg.socket(i).connect_or_bind() == protobuf::ZeroMQNodeConfig::Socket::CONNECT)
        {
            std::string endpoint;
            switch(cfg.socket(i).transport())
            {
                case protobuf::ZeroMQNodeConfig::Socket::INPROC:
                    endpoint = "inproc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::IPC:
                    endpoint = "ipc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::TCP:
                    endpoint = "tcp://" + cfg.socket(i).ethernet_address() + ":"
                        + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::PGM:
                    endpoint = "pgm://" + cfg.socket(i).ethernet_address() + ";"
                        + cfg.socket(i).multicast_address() + ":" + as<std::string>(cfg.socket(i).ethernet_port());
                break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::EPGM:
                    endpoint = "epgm://" + cfg.socket(i).ethernet_address() + ";"
                        + cfg.socket(i).multicast_address() + ":" + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
            }

            try
            {
                this_socket->connect(endpoint.c_str());
                glog.is(debug1) && glog << cfg.socket(i).ShortDebugString() << " connected to endpoint - " << endpoint << std::endl;
            }    
            catch(std::exception& e)
            {
                glog.is(die) &&
                    glog << "cannot connect to: " << endpoint << ": " << e.what() << std::endl;
            }
        }
        else if(cfg.socket(i).connect_or_bind() == protobuf::ZeroMQNodeConfig::Socket::BIND)
        {
            std::string endpoint;
            switch(cfg.socket(i).transport())
            {
                case protobuf::ZeroMQNodeConfig::Socket::INPROC:
                    endpoint = "inproc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::IPC:
                    endpoint = "ipc://" + cfg.socket(i).socket_name();
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::TCP:
                    endpoint = "tcp://*:" + as<std::string>(cfg.socket(i).ethernet_port());
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::PGM:
                    throw(goby::Exception("Cannot BIND to PGM socket (use CONNECT)"));
                    break;
                    
                case protobuf::ZeroMQNodeConfig::Socket::EPGM:
                    throw(goby::Exception("Cannot BIND to EPGM socket (use CONNECT)"));
                    break;
            }            

            try
            {
                this_socket->bind(endpoint.c_str());
                glog.is(debug1) && glog << cfg.socket(i).ShortDebugString() << "bound to endpoint - " << endpoint << std::endl;
            }    
            catch(std::exception& e)
            {
                glog.is(die) &&
                    glog << "cannot bind to: " << endpoint << ": " << e.what() << std::endl;
            }

        }
    }
}

goby::core::ZeroMQNode::~ZeroMQNode()
{
}

int goby::core::ZeroMQNode::socket_type(protobuf::ZeroMQNodeConfig::Socket::SocketType type)
{
    switch(type)
    {
        case protobuf::ZeroMQNodeConfig::Socket::PUBLISH: return ZMQ_PUB;
        case protobuf::ZeroMQNodeConfig::Socket::SUBSCRIBE: return ZMQ_SUB;
        case protobuf::ZeroMQNodeConfig::Socket::REPLY: return ZMQ_REP;
        case protobuf::ZeroMQNodeConfig::Socket::REQUEST: return ZMQ_REQ;
//        case protobuf::ZeroMQNodeConfig::Socket::ZMQ_PUSH: return ZMQ_PUSH;
//        case protobuf::ZeroMQNodeConfig::Socket::ZMQ_PULL: return ZMQ_PULL;
//        case protobuf::ZeroMQNodeConfig::Socket::ZMQ_DEALER: return ZMQ_DEALER;
//        case protobuf::ZeroMQNodeConfig::Socket::ZMQ_ROUTER: return ZMQ_ROUTER;
    }
    throw(goby::Exception("Invalid SocketType"));
}

boost::shared_ptr<zmq::socket_t> goby::core::ZeroMQNode::socket_from_id(int socket_id)
{
    std::map<int, boost::shared_ptr<zmq::socket_t> >::iterator it = sockets_.find(socket_id);
    if(it != sockets_.end())
        return it->second;
    else
        throw(goby::Exception("Attempted to access socket_id " + as<std::string>(socket_id) + " which does not exist"));
}

void goby::core::ZeroMQNode::subscribe_all(int socket_id)
{
    socket_from_id(socket_id)->setsockopt(ZMQ_SUBSCRIBE, 0, 0);
}
void goby::core::ZeroMQNode::subscribe(MarshallingScheme marshalling_scheme,
                                       const std::string& identifier,
                                       int socket_id)
{
    pre_subscribe_hooks(marshalling_scheme, identifier, socket_id);
    
    std::string zmq_filter = make_header(marshalling_scheme, identifier);
    int NULL_TERMINATOR_SIZE = 1;
    zmq_filter.resize(zmq_filter.size() - NULL_TERMINATOR_SIZE);
    socket_from_id(socket_id)->setsockopt(ZMQ_SUBSCRIBE, zmq_filter.c_str(), zmq_filter.size());
    
    glog.is(debug1) && glog << "Subscribed for marshalling " << marshalling_scheme << " with identifier: " << identifier << "using zmq_filter: " << goby::util::hex_encode(zmq_filter) << std::endl;

        
    post_send_hooks(marshalling_scheme, identifier, socket_id);
}

void goby::core::ZeroMQNode::send(MarshallingScheme marshalling_scheme,
                                  const std::string& identifier,
                                  const void* body_data,
                                  int body_size,
                                  int socket_id)
{
    pre_send_hooks(marshalling_scheme, identifier, socket_id);
    
    std::string header = make_header(marshalling_scheme, identifier);

    zmq::message_t msg(header.size() + body_size);
    memcpy(msg.data(), header.c_str(), header.size()); // insert header
    memcpy(static_cast<char*>(msg.data()) + header.size(), body_data, body_size); // insert body

    glog.is(debug2) &&
        glog << "message hex: " << hex_encode(std::string(static_cast<const char*>(msg.data()),msg.size())) << std::endl;
    socket_from_id(socket_id)->send(msg);

    post_send_hooks(marshalling_scheme, identifier, socket_id);
}


void goby::core::ZeroMQNode::handle_receive(const void* data,
                                            int size,
                                            int message_part,
                                            int socket_id)
{
    std::string bytes(static_cast<const char*>(data),
                      size);
    

    glog.is(debug2) &&
        glog << "got a message: " << goby::util::hex_encode(bytes) << std::endl;
    
    
    static MarshallingScheme marshalling_scheme = MARSHALLING_UNKNOWN;
    static std::string identifier;

    switch(message_part)
    {
        case 0:
        {
            // byte size of marshalling id
            const unsigned MARSHALLING_SIZE = BITS_IN_UINT32 / BITS_IN_BYTE;

            if(bytes.size() < MARSHALLING_SIZE)
                throw(std::runtime_error("Message is too small"));

            
            google::protobuf::uint32 marshalling_int = 0;
            for(int i = MARSHALLING_SIZE-1, n = 0; i >= n; --i)
            {
                marshalling_int <<= BITS_IN_BYTE;
                marshalling_int ^= bytes[i];
            }
        
            marshalling_int = boost::asio::detail::socket_ops::network_to_host_long(
                marshalling_int);                 
        
            if(marshalling_int >= MARSHALLING_UNKNOWN &&
               marshalling_int <= MARSHALLING_MAX)
                marshalling_scheme = static_cast<MarshallingScheme>(marshalling_int);
            else
                throw(std::runtime_error("Invalid marshalling value = "
                                         + as<std::string>(marshalling_int)));
        
            
            identifier = bytes.substr(MARSHALLING_SIZE,
                                      bytes.find('\0', MARSHALLING_SIZE)-MARSHALLING_SIZE);

            glog.is(debug1) &&
                glog << "Got message of type: [" << identifier << "]" << std::endl;

            // +1 for null terminator
            const int HEADER_SIZE = MARSHALLING_SIZE+identifier.size() + 1;
            std::string body(static_cast<const char*>(data)+HEADER_SIZE,
                             size-HEADER_SIZE);
            
            glog.is(debug2) &&
                glog << "Body [" << goby::util::hex_encode(body)<< "]" << std::endl;
            
            inbox_signal_(marshalling_scheme,
                          identifier,
                          static_cast<const char*>(data)+HEADER_SIZE,
                          size-HEADER_SIZE,
                          socket_id);
        }
        break;
            
            
        default:
            throw(std::runtime_error("Got more parts to the message than expecting (expecting only 1)"));    
            break;
    }
}

bool goby::core::ZeroMQNode::poll(long timeout /* = -1 */)
{
    glog.is(debug2) && glog << "Have " << poll_items_.size() << " items to poll" << std::endl;   
    bool had_events = false;
    zmq::poll (&poll_items_[0], poll_items_.size(), timeout);
    for(int i = 0, n = poll_items_.size(); i < n; ++i)
    {
        if (poll_items_[i].revents & ZMQ_POLLIN) 
        {
            int message_part = 0;
            int64_t more;
            size_t more_size = sizeof more;
            do {
                /* Create an empty ØMQ message to hold the message part */
                zmq_msg_t part;
                int rc = zmq_msg_init (&part);
                // assert (rc == 0);
                /* Block until a message is available to be received from socket */
                rc = zmq_recv (poll_items_[i].socket, &part, 0);
                glog.is(debug2) && glog << "Had event for poll item " << i << std::endl;
                poll_callbacks_[i](zmq_msg_data(&part), zmq_msg_size(&part), message_part);
                // assert (rc == 0);
                /* Determine if more message parts are to follow */
                rc = zmq_getsockopt (poll_items_[i].socket, ZMQ_RCVMORE, &more, &more_size);
                // assert (rc == 0);
                zmq_msg_close (&part);
                ++message_part;
            } while (more);
            had_events = true;
        }
    }
    return had_events;
}


std::string goby::core::ZeroMQNode::make_header(MarshallingScheme marshalling_scheme,
                                                const std::string& identifier)
{
    std::string zmq_filter;
    
    google::protobuf::uint32 marshalling_int = boost::asio::detail::socket_ops::host_to_network_long(static_cast<google::protobuf::uint32>(marshalling_scheme));
    
    for(int i = 0, n = BITS_IN_UINT32 / BITS_IN_BYTE; i < n; ++i)
    {
        zmq_filter.push_back(marshalling_int & 0xFF);
        marshalling_int >>= BITS_IN_BYTE;
    }
    zmq_filter += identifier + '\0';

    glog.is(debug2) &&
        glog << "zmq header: " << goby::util::hex_encode(zmq_filter) << std::endl;

    return zmq_filter;
}
