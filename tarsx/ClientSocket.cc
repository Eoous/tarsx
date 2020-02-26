#include "ClientSocket.h"

using namespace tarsx;

EndPoint::EndPoint(const std::string& host, int port)
	:host_(host),port_(port){
	
}

EndPoint::EndPoint(const EndPoint& end_point)
	:host_(end_point.host_),port_(end_point.port_){
}

EndPoint& EndPoint::operator=(const EndPoint& end_point) {
	if(this != &end_point) {
		host_ = end_point.host_;
		port_ = end_point.port_;
	}
	return *this;
}
