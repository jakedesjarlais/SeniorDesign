#ifndef IBTCP_H
#define IBTCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>
#include <infiniband/verbs.h>
#define DEFAULT_BUF_SIZE 65536
#define DEFAULT_TX_DEPTH 100
#define DEFAULT_IB_PORT 1
#define DEFAULT_SL 1
#define RDMA_WRID 3



struct app_context{
	struct ibv_context 		*ib_context;
	struct ibv_pd      		*pd;
	struct ibv_mr      		*mr;
	struct ibv_cq      		*rcq;
	struct ibv_cq      		*scq;
	struct ibv_qp      		*qp;
	struct ibv_comp_channel *ch;
	void               		*buf;
	struct ibv_sge      	sge_list;
	struct ibv_send_wr  	wr;
};

struct ib_connection {
    int             	lid;
    int            	 	qpn;
    int             	psn;
	unsigned 			rkey;
	unsigned long long 	vaddr;
	union ibv_gid		gid;
};

struct app_data {
	int							port;
	int 		    			sockfd;
	char						*servername;
	struct ib_connection		local_connection;
	struct ib_connection 		*remote_connection;
	struct ibv_device			*ib_dev;

};

static int tcp_client_connect(struct app_data *data);
static int tcp_server_listen(struct app_data *data);

int init_context(struct app_context **context, struct app_data *data);
//static void destroy_ctx(struct app_context *context);

static int set_local_ib_connection(struct app_context *context, struct app_data *data);
static void print_ib_connection(char *conn_name, struct ib_connection *conn);

static int tcp_exch_ib_connection_info(struct app_data *data);

static int qp_change_state_init(struct ibv_qp *qp);
static int qp_change_state_rtr(struct ibv_qp *qp, struct app_data *data);
static int qp_change_state_rts(struct ibv_qp *qp, struct app_data *data);

static int rdma_write(struct app_context *context, struct app_data *data);



#endif
