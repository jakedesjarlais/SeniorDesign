#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

//Preloaded functions of librdmacm/include/rdma/rdma_cma.h
//will append to (else create and append to) a file called 'data.log'
FILE *fp;

enum rdma_port_space {
	RDMA_PS_IPOIB = 0x0002,
	RDMA_PS_TCP   = 0x0106,
	RDMA_PS_UDP   = 0x0111,
	RDMA_PS_IB    = 0x013F,
};

enum rdma_cm_event_type {
	RDMA_CM_EVENT_ADDR_RESOLVED,
	RDMA_CM_EVENT_ADDR_ERROR,
	RDMA_CM_EVENT_ROUTE_RESOLVED,
	RDMA_CM_EVENT_ROUTE_ERROR,
	RDMA_CM_EVENT_CONNECT_REQUEST,
	RDMA_CM_EVENT_CONNECT_RESPONSE,
	RDMA_CM_EVENT_CONNECT_ERROR,
	RDMA_CM_EVENT_UNREACHABLE,
	RDMA_CM_EVENT_REJECTED,
	RDMA_CM_EVENT_ESTABLISHED,
	RDMA_CM_EVENT_DISCONNECTED,
	RDMA_CM_EVENT_DEVICE_REMOVAL,
	RDMA_CM_EVENT_MULTICAST_JOIN,
	RDMA_CM_EVENT_MULTICAST_ERROR,
	RDMA_CM_EVENT_ADDR_CHANGE,
	RDMA_CM_EVENT_TIMEWAIT_EXIT
};

/*********Timer Functions**********/

uint64_t getTime(void){
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t time = 1000000 * t.tv_sec + t.tv_usec;
    return time;
}

/**********Logging Functions**********/

void startLog(char* func){
    fp = fopen("data.log", "a");
    fputs("Starting timer for ", fp);
    fputs(func, fp);
    fputs("\n", fp);
    fclose(fp);
}

void endLog(char* func, uint64_t time){
    fp = fopen("data.log", "a");
    fputs(func, fp);
    fprintf(fp, " took %d microseconds\n", time);
    fclose(fp);
}

/**********Preloaded Functions**********/

/*initiate an active connection request*/

typedef int (*orig_rdma_connect_type)(struct rdma_cm_id *id, struct rdma_conn_param *conn_param);

int rdma_connect(struct rdma_cm_id *id, struct rdma_conn_param *conn_param){
    startLog("rdma_connect");
    uint64_t startTime = getTime();

    orig_rdma_connect_type orig_rdma_connect;
    orig_rdma_connect = (orig_rdma_connect_type)dlsym(RTLD_NEXT,"rdma_connect");
    int tmp = orig_rdma_connect(id, conn_param);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_connect", timeElapsed);
    return tmp;
}

/*allocate a QP*/

typedef int (*orig_rdma_create_qp_type)(struct rdma_cm_id *id, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr);

int rdma_create_qp(struct rdma_cm_id *id, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr){
    startLog("rdma_create_qp");
    uint64_t startTime = getTime();
    
    orig_rdma_create_qp_type orig_rdma_create_qp;
    orig_rdma_create_qp = (orig_rdma_create_qp_type)dlsym(RTLD_NEXT,"rdma_create_qp");
    int tmp = orig_rdma_create_qp(id, pd, qp_init_attr);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_create_qp", timeElapsed);
    return tmp;
}

/*allocate a QP(ex)*/

typedef int (*orig_rdma_create_qp_ex_type)(struct rdma_cm_id *id, struct ibv_qp_init_attr_ex *qp_init_attr);

int rdma_create_qp_ex(struct rdma_cm_id *id, struct ibv_qp_init_attr_ex *qp_init_attr){
    startLog("rdma_create_qp_ex");
    uint64_t startTime = getTime();
    
    orig_rdma_create_qp_ex_type orig_rdma_create_qp_ex;
    orig_rdma_create_qp_ex = (orig_rdma_create_qp_ex_type)dlsym(RTLD_NEXT,"rdma_create_qp_ex");
    int tmp = orig_rdma_create_qp_ex(id, qp_init_attr);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_create_qp_ex", timeElapsed);
    return tmp;
}

/*resolve the route information needed to establish a connection*/

typedef int (*orig_rdma_resolve_route_type)(struct rdma_cm_id *id, int timeout_ms);

int rdma_resolve_route(struct rdma_cm_id *id, int timeout_ms){
    startLog("rdma_resolve_route");
    uint64_t startTime = getTime();
    
    orig_rdma_resolve_route_type orig_rdma_resolve_route;
    orig_rdma_resolve_route = (orig_rdma_resolve_route_type)dlsym(RTLD_NEXT,"rdma_resolve_route");
    int tmp = orig_rdma_resolve_route(id, timeout_ms);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_resolve_route", timeElapsed);
    return tmp;
}

/*resolve destination and optional source addresses*/

typedef int (*orig_rdma_resolve_addr_type)(struct rdma_cm_id *id, struct sockaddr *src_addr, struct sockaddr *dst_addr, int timeout_ms);

int rdma_resolve_addr(struct rdma_cm_id *id, struct sockaddr *src_addr, struct sockaddr *dst_addr, int timeout_ms){
    startLog("rdma_resolve_addr");
    uint64_t startTime = getTime();
    
    orig_rdma_resolve_addr_type orig_rdma_resolve_addr;
    orig_rdma_resolve_addr = (orig_rdma_resolve_addr_type)dlsym(RTLD_NEXT,"rdma_resolve_addr");
    int tmp = orig_rdma_resolve_addr(id, src_addr, dst_addr, timeout_ms);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_resolve_addr", timeElapsed);
    return tmp;
}

/*bind an RDMA identifier to a source address*/

typedef int (*orig_rdma_bind_addr_type)(struct rdma_cm_id *id, struct sockaddr *addr);

int rdma_bind_addr(struct rdma_cm_id *id, struct sockaddr *addr){
    startLog("rdma_bind_addr");
    uint64_t startTime = getTime();
    
    orig_rdma_bind_addr_type orig_rdma_bind_addr;
    orig_rdma_bind_addr = (orig_rdma_bind_addr_type)dlsym(RTLD_NEXT,"rdma_bind_addr");
    int tmp = orig_rdma_bind_addr(id, addr);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_bind_addr", timeElapsed);
    return tmp;
}

/*allocate a communication identifier and qp*/

typedef int (*orig_rdma_create_ep_type)(struct rdma_cm_id **id, struct rdma_addrinfo *res, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr);

int rdma_create_ep(struct rdma_cm_id **id, struct rdma_addrinfo *res, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr){
    startLog("rdma_create_ep");
    uint64_t startTime = getTime();
    
    orig_rdma_create_ep_type orig_rdma_create_ep;
    orig_rdma_create_ep = (orig_rdma_create_ep_type)dlsym(RTLD_NEXT,"rdma_create_ep");
    int tmp = orig_rdma_create_ep(id, res, pd, qp_init_attr);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_create_ep", timeElapsed);
    return tmp;
}

/*allocate a communication identifier*/

typedef int (*orig_rdma_create_id_type)(struct rdma_event_channel *channel, struct rdma_cm_id **id, void *context, enum rdma_port_space ps);

int rdma_create_id(struct rdma_event_channel *channel, struct rdma_cm_id **id, void *context, enum rdma_port_space ps){
    startLog("rdma_create_id");
    uint64_t startTime = getTime();
    
    orig_rdma_create_id_type orig_rdma_create_id;
    orig_rdma_create_id = (orig_rdma_create_id_type)dlsym(RTLD_NEXT,"rdma_create_id");
    int tmp = orig_rdma_create_id(channel, id, context, ps);

    uint64_t timeElapsed = getTime() - startTime;
    endLog("rdma_create_id", timeElapsed);
    return tmp;
}

/*
//open a channel used to report communication events
typedef struct (*orig_rdma_event_channel_type)(void);

struct rdma_event_channel(void){
    startLog("rdma_event_channel");
    startTimer();
    orig_rdma_event_channel_type orig_rdma_event_channel;
    orig_rdma_event_channel = (orig_rdma_event_channel_type)dlsym(RTLD_NEXT,"rdma_event_channel");
    struct tmp = orig_rdma_event_channel();
    uint64_t timeElapsed = getTimer();
    endLog("rdma_event_channel", timeElapsed);
    return tmp;
}

*/
