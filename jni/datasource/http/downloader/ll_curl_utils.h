#ifndef WRAPPER_UTILS_H_
#define WRAPPER_UTILS_H_

#include <string>
#include <map>

// Get the network interface receiving bytes
using std::string;
using std::map;


//set callbacks for openssl
int dl_openssl_thread_setup(void);
//clean callbacks for openssl
int dl_openssl_thread_cleanup(void);

string dl_getHostPort(const string &strUrl);

string dl_getHostname(const string &strUrl);

string dl_getProtocol(const string &strUrl);

string dl_getPort(const string &strUrl, std::map<std::string, std::string> &WRAPPER_Protocol);

string dl_getIpStr(void *dlcurl, const string &hostname);

#endif /* WRAPPER_UTILS_H_ */
