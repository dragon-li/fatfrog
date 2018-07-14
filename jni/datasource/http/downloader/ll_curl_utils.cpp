#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include "openssl/crypto.h"
#include <algorithm>
#include <string>
#include "dl_log.h"
#include "ll_curl_utils.h"
#include <ctype.h>


#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID        pthread_self()

// set callbacks for OpenSSL to ensure thread safety

static MUTEX_TYPE *mutex_buf = NULL;

static void locking_function(int mode, int n, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
    return ((unsigned long) THREAD_ID);
}

struct CRYPTO_dynlock_value
{
    MUTEX_TYPE mutex;
};
static struct CRYPTO_dynlock_value * dyn_create_function(const char *file, int line)
{
    struct CRYPTO_dynlock_value *value;
    value = (struct CRYPTO_dynlock_value *) malloc(sizeof(struct CRYPTO_dynlock_value));
    if (!value)
        return NULL;
    MUTEX_SETUP(value->mutex);
    return value;
}
static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(l->mutex);
    else
        MUTEX_UNLOCK(l->mutex);
}
static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
    MUTEX_CLEANUP(l->mutex);
    free(l);
}

int dl_openssl_thread_setup(void)
{
    return 0;
}

int dl_openssl_thread_cleanup(void)
{
        return -1;
}


string dl_getHostPort(const string &strUrl)
{
    string hn;
    std::size_t bIdx = strUrl.find("://");
    std::size_t eIdx = std::string::npos;
    if(bIdx!= std::string::npos){
        bIdx += 3;
        eIdx = strUrl.find("/",bIdx);
        if(eIdx != std::string::npos){
            hn = strUrl.substr(bIdx,eIdx-bIdx);
        } else {
            hn = strUrl.substr(bIdx);
        }
    }
    std::transform(hn.begin(),hn.end(),hn.begin(),::tolower);
    return hn;
}
string dl_getHostname(const string &strUrl)
{
    string hn = dl_getHostPort(strUrl);
    if(!hn.empty()){
        std::size_t bIdx = hn.find(":");
        if(bIdx != std::string::npos){
            hn = hn.substr(0,bIdx);
        }
    }
    return hn;
}
string dl_getProtocol(const string &strUrl)
{
    string proto;
    std::size_t bIdx = strUrl.find("://");
    if(bIdx!= std::string::npos){
        proto = strUrl.substr(0,bIdx);
    }
	
    std::transform(proto.begin(),proto.end(),proto.begin(),::tolower);
    return proto;
}
string dl_getPort(const string &strUrl, std::map<std::string, std::string> &WRAPPER_Protocol)
{
    string hn = dl_getHostPort(strUrl);
    string port = "";
    if( !hn.empty()){
        std::size_t bIdx = hn.find(":");
        if(bIdx != std::string::npos){
            port = hn.substr(bIdx+1);
            std::string::iterator it = port.begin();
            for(;it != port.end(); it ++) {
                if(!isdigit(*it)) break;
            }
            if(it != port.end()){
                port = "";
            }
        }
        if( port.empty() ){
            std::map<string,string>::iterator it = WRAPPER_Protocol.find(dl_getProtocol(strUrl));
            if(it != WRAPPER_Protocol.end()){
                port = it->second;
            }
        }
    }
    return port;
}

#include <sys/syscall.h>
static uint32_t dl_gettid(void)
{
    uint32_t id = 0;
    return id;
}

