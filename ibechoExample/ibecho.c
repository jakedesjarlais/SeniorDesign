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

static pid_t pid;

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


int init_context(struct app_context **context, struct app_data *data)
{
	//ASSERT(!context);
	
	*context = malloc(sizeof **context);
	if (!(*context)) {
		return -ENOMEM;
	}
	memset(*context, 0, sizeof **context);
	
	// Allocate page aligned buffer
	posix_memalign(&(*context)->buf, 4096, DEFAULT_BUF_SIZE * 2);
	if (!(*context)->buf) {
		fprintf(stderr, "Err in posix_memalign()\n");
		return -ENOMEM;
	}
	memset((*context)->buf, 0, DEFAULT_BUF_SIZE * 2);
	
	struct ibv_device **dev_list;

	dev_list = ibv_get_device_list(NULL);
	if (!dev_list) {
		fprintf(stderr, "No IB-device available. get_device_list returned NULL\n");
		return -1;
	}

	data->ib_dev = dev_list[0];
	if (!data->ib_dev) {
        fprintf(stderr, "IB-device could not be assigned. Maybe dev_list array is empty\n");
		return -1;
	}

	(*context)->ib_context = ibv_open_device(data->ib_dev);
	if (!(*context)->ib_context) {
		fprintf(stderr, "Could not create context, ibv_open_device\n");
		return -1;
	}

	(*context)->pd = ibv_alloc_pd((*context)->ib_context);
	if (!(*context)->ib_context) {
		fprintf(stderr, "Could not allocate protection domain, ibv_alloc_pd\n");
		return -1;
	}
	
	(*context)->mr = ibv_reg_mr((*context)->pd, (*context)->buf, DEFAULT_BUF_SIZE * 2, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
	if (!(*context)->mr) {
		fprintf(stderr, "Could not allocate mr, ibv_reg_mr. Do you have root access?\n");
		return -1;
	}
	
	//TEST_Z(ctx->ch = ibv_create_comp_channel(ctx->context),
	//		"Could not create completion channel, ibv_create_comp_channel");

	(*context)->rcq = ibv_create_cq((*context)->ib_context, 1, NULL, NULL, 0);
	if (!(*context)->rcq) {
		fprintf(stderr, "Could not create receive completion queue, ibv_create_cq\n");
 		return -1;
	}

	(*context)->scq = ibv_create_cq((*context)->ib_context, DEFAULT_TX_DEPTH, *context, NULL, 0);
	if (!(*context)->scq) {
		fprintf(stderr, "Could not create send completion queue, ibv_create_cq\n");
 		return -1;
	}
	
	struct ibv_qp_init_attr qp_init_attr = {
		.send_cq = (*context)->scq,
		.recv_cq = (*context)->rcq,
		.qp_type = IBV_QPT_RC,
		.cap = {
			.max_send_wr = DEFAULT_TX_DEPTH,
			.max_recv_wr = 1,
			.max_send_sge = 1,
			.max_recv_sge = 1,
			.max_inline_data = 0
		}
	};

	(*context)->qp = ibv_create_qp((*context)->pd, &qp_init_attr);
	if (!(*context)->qp) {
		fprintf(stderr, "Could not create queue pair, ibv_create_qp\n");	
 		return -1;
	}
	
	int err = qp_change_state_init((*context)->qp);
	if (err) {
		fprintf(stderr, "qp_change_state_init failed: %d\n", err);
		return err;
	}	
	return 0;
}

static int qp_change_state_init(struct ibv_qp *qp){
	
	struct ibv_qp_attr *attr;
	int err = 0;
	
    attr =  malloc(sizeof *attr);
    memset(attr, 0, sizeof *attr);

    attr->qp_state        	= IBV_QPS_INIT;
    attr->pkey_index      	= 0;
    attr->port_num        	= DEFAULT_IB_PORT;
    attr->qp_access_flags	= IBV_ACCESS_REMOTE_WRITE;

    err = ibv_modify_qp(qp, attr,
                            IBV_QP_STATE        |
                            IBV_QP_PKEY_INDEX   |
                            IBV_QP_PORT         |
                            IBV_QP_ACCESS_FLAGS);
	if (err) {
		fprintf(stderr, "Could not modify QP to INIT, ibv_modify_qp, err: %d\n", err);
	}

	return err;
}

/*
 *  set_local_ib_connection
 * *************************
 *  Sets all relevant attributes needed for an IB connection. Those are then sent to the peer via TCP
 * 	Information needed to exchange data over IB are: 
 *	  lid - Local Identifier, 16 bit address assigned to end node by subnet manager 
 *	  qpn - Queue Pair Number, identifies qpn within channel adapter (HCA)
 *	  psn - Packet Sequence Number, used to verify correct delivery sequence of packages (similar to ACK)
 *	  rkey - Remote Key, together with 'vaddr' identifies and grants access to memory region
 *	  vaddr - Virtual Address, memory address that peer can later write to
 */
static int set_local_ib_connection(struct app_context *context, struct app_data *data){

	// First get local lid
	struct ibv_port_attr attr;
	int err = ibv_query_port(context->ib_context, DEFAULT_IB_PORT, &attr);
	if (err) {
		fprintf(stderr, "Could not get port attributes, ibv_query_port\n");
		return err;
	}
	
	union ibv_gid gid;
	err = ibv_query_gid(context->ib_context, DEFAULT_IB_PORT, 0, &gid);
	if (err) {
		fprintf(stderr, "Could not get port gid, ibv_query_gid\n");
		return err;
	}
	
	data->local_connection.lid = attr.lid;
	data->local_connection.qpn = context->qp->qp_num;
	data->local_connection.psn = lrand48() & 0xffffff;
	data->local_connection.rkey = context->mr->rkey;
	data->local_connection.vaddr = (uintptr_t)context->buf + DEFAULT_BUF_SIZE;
	data->local_connection.gid = gid;
	
	return 0;
}

static int tcp_exch_ib_connection_info(struct app_data *data){

	char msg[sizeof "0000:000000:000000:00000000:0000000000000000:0000000000000000:0000000000000000"];
	int parsed;

	struct ib_connection *local = &data->local_connection;

	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx:%016Lx:%016Lx", 
				local->lid, local->qpn, local->psn, local->rkey, local->vaddr, local->gid.global.interface_id, local->gid.global.subnet_prefix);

	if (write(data->sockfd, msg, sizeof msg) != sizeof msg){
		fprintf(stderr, "Could not send connection_details to peer\n");
		return -1;
	}	

	if (read(data->sockfd, msg, sizeof msg) != sizeof msg){
		fprintf(stderr, "Could not receive connection_details to peer\n");
		return -1;
	}

	data->remote_connection = malloc(sizeof(struct ib_connection));
	if (!data->remote_connection) {
		fprintf(stderr, "Could not allocate memory for remote_connection connection\n");
		return -ENOMEM;
	}

	struct ib_connection *remote = data->remote_connection;

	parsed = sscanf(msg, "%x:%x:%x:%x:%Lx:%Lx:%Lx", 
						&remote->lid, &remote->qpn, &remote->psn, &remote->rkey, &remote->vaddr, &remote->gid.global.interface_id, &remote->gid.global.subnet_prefix);
	
	if(parsed != 7){
		fprintf(stderr, "Could not parse message from peer, items parsed: %d\n", parsed);
	}
	
	return 0;
}

static void print_ib_connection(char *conn_name, struct ib_connection *conn) {
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x RKey %#08x VAddr %#016Lx gid %#016Lx%#016Lx\n", 
			conn_name, conn->lid, conn->qpn, conn->psn, conn->rkey, conn->vaddr, conn->gid.global.interface_id, conn->gid.global.subnet_prefix);

}

/*
 *  qp_change_state_rtr
 * **********************
 *  Changes Queue Pair status to RTR (Ready to receive)
 */
static int qp_change_state_rtr(struct ibv_qp *qp, struct app_data *data) {
	
	struct ibv_qp_attr *attr;
	int err = 0;
	
    attr =  malloc(sizeof *attr);
	if (!attr) {
		fprintf(stderr, "Could not allocate memory for ibv_qp_attr\n");
		return -ENOMEM;		
	}
    memset(attr, 0, sizeof *attr);

    attr->qp_state              = IBV_QPS_RTR;
    attr->path_mtu              = IBV_MTU_1024;
    attr->dest_qp_num           = data->remote_connection->qpn;
    attr->rq_psn                = data->remote_connection->psn;
    attr->max_dest_rd_atomic    = 0;
    attr->min_rnr_timer         = 2;
    attr->ah_attr.is_global     = 1;
    attr->ah_attr.dlid          = data->remote_connection->lid;
    attr->ah_attr.sl            = DEFAULT_SL;
    attr->ah_attr.src_path_bits = 0;
    attr->ah_attr.port_num      = DEFAULT_IB_PORT;
	
	attr->ah_attr.grh.dgid = data->remote_connection->gid;
	attr->ah_attr.grh.flow_label = 0;
	attr->ah_attr.grh.sgid_index = 0;
	attr->ah_attr.grh.hop_limit = 1;
	attr->ah_attr.grh.traffic_class = 0;

    err = ibv_modify_qp(qp, attr,
                IBV_QP_STATE                |
                IBV_QP_AV                   |
                IBV_QP_PATH_MTU             |
                IBV_QP_DEST_QPN             |
                IBV_QP_RQ_PSN               |
                IBV_QP_MAX_DEST_RD_ATOMIC   |
                IBV_QP_MIN_RNR_TIMER);
	if (err) {
		fprintf(stderr, "Could not modify QP to RTR state: %d\n", err);
		return err;		
	}	

	free(attr);
	
	return err;
}

/*
 *  qp_change_state_rts
 * **********************
 *  Changes Queue Pair status to RTS (Ready to send)
 *	QP status has to be RTR before changing it to RTS
 */
static int qp_change_state_rts(struct ibv_qp *qp, struct app_data *data) {
	struct ibv_qp_attr *attr;
	int err = 0;
	
    attr =  malloc(sizeof *attr);
    memset(attr, 0, sizeof *attr);

	attr->qp_state              = IBV_QPS_RTS;
    attr->timeout               = 14;
    attr->retry_cnt             = 7;
    attr->rnr_retry             = 7;    /* infinite retry */
    attr->sq_psn                = data->local_connection.psn;
    attr->max_rd_atomic         = 1;

    err = ibv_modify_qp(qp, attr,
                IBV_QP_STATE            |
                IBV_QP_TIMEOUT          |
                IBV_QP_RETRY_CNT        |
                IBV_QP_RNR_RETRY        |
                IBV_QP_SQ_PSN           |
                IBV_QP_MAX_QP_RD_ATOMIC);
	if (err) {
		fprintf(stderr, "Could not modify QP to RTS state: %d\n", err);
		return err;		
	}	

	free(attr);
	
	return err;
}

/*
 *  rdma_write
 * **********************
 *	Writes 'context-buf' into buffer of peer
 */
static int rdma_write(struct app_context *context, struct app_data *data){
	
	context->sge_list.addr      = (uintptr_t)context->buf;
   	context->sge_list.length    = DEFAULT_BUF_SIZE;
   	context->sge_list.lkey      = context->mr->lkey;

  	context->wr.wr.rdma.remote_addr = data->remote_connection->vaddr;
    context->wr.wr.rdma.rkey        = data->remote_connection->rkey;
    context->wr.wr_id       = RDMA_WRID;
    context->wr.sg_list     = &context->sge_list;
    context->wr.num_sge     = 1;
    context->wr.opcode      = IBV_WR_RDMA_WRITE;
    context->wr.send_flags  = IBV_SEND_SIGNALED;
    context->wr.next        = NULL;

    struct ibv_send_wr *bad_wr;
	int err = 0;
	
    err = ibv_post_send(context->qp, &context->wr, &bad_wr);
	if (err) {
		fprintf(stderr, "ibv_post_send failed. This is bad mkey: %d\n", err);
		return err;		
	}
	
	// yuvaldeg
	// Conrols if message was competely sent. But fails if client destroys his context too early. This would have to
	// be timed by the server telling the client that the rdma_write has been completed.
	/*
    int ne;
    struct ibv_wc wc;

    do {
        ne = ibv_poll_cq(ctx->scq, 1, &wc);
    } while(ne == 0);

    if (ne < 0) {
        fprintf(stderr, "%s: poll CQ failed %d\n",
            __func__, ne);
    }

    if (wc.status != IBV_WC_SUCCESS) {
            fprintf(stderr, "%d:%s: Completion with error at %s:\n",
                pid, __func__, data->servername ? "client" : "server");
            fprintf(stderr, "%d:%s: Failed status %d: wr_id %d\n",
                pid, __func__, wc.status, (int) wc.wr_id);
        }

    if (wc.status == IBV_WC_SUCCESS) {
        printf("wrid: %i successfull\n",(int)wc.wr_id);
        printf("%i bytes transfered\n",(int)wc.byte_len);
    }
	*/
	
	return 0;
}

/*
 *  tcp_client_connect
 * ********************
 *	Creates a connection to a TCP server 
 */
static int tcp_client_connect(struct app_data *data)
{
	struct addrinfo *res, *t;
	struct addrinfo hints = {
		.ai_family		= AF_UNSPEC,
		.ai_socktype	= SOCK_STREAM
	};

	char *service;
//	int n;
	int sockfd = -1;
//	struct sockaddr_in sin;
	int err = 0;

	err = asprintf(&service, "%d", data->port);
    if (!err || err == -1) {
		fprintf(stderr, "Error writing port-number to port-string, %d\n", err);
		return err;		
	}
			
	err = getaddrinfo(data->servername, service, &hints, &res);
    if (err < 0) {
		fprintf(stderr, "getaddrinfo threw error: %d\n", err);
		return err;		
	}
	
	for(t = res; t; t = t->ai_next){
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
		if (!sockfd) {
			fprintf(stderr, "Could not create client socket\n");
			return -1;
		}
		
		err = connect(sockfd,t->ai_addr, t->ai_addrlen);
		if (err) {
			fprintf(stderr, "Could not connect %d\n", err);
			return err;
		}
	}
	
	freeaddrinfo(res);
	
	return sockfd;
}

/*
 *  tcp_server_listen
 * *******************
 *  Creates a TCP server socket  which listens for incoming connections 
 */
static int tcp_server_listen(struct app_data *data){
	struct addrinfo *res;//, *t;
	struct addrinfo hints = {
		.ai_flags		= AI_PASSIVE,
		.ai_family		= AF_UNSPEC,
		.ai_socktype	= SOCK_STREAM	
	};

	char *service;
	int sockfd = -1;
	int n, connfd;
//	struct sockaddr_in sin;
	int err =0;
	
	err = asprintf(&service, "%d", data->port);
    if (!err || err == -1) {
		fprintf(stderr, "Error writing port-number to port-string, %d\n", err);
		return err;		
	}
	
    err = getaddrinfo(NULL, service, &hints, &res);
    if (err < 0) {
		fprintf(stderr, "getaddrinfo threw error: %d\n", err);
		return -1;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!sockfd) {
		fprintf(stderr, "Could not create server socket\n");
		return -1;
	}
	
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

    err = bind(sockfd,res->ai_addr, res->ai_addrlen);
    if (err) {
		fprintf(stderr, "Could not bind addr to socket\n");
		return -1;
	}
	
	listen(sockfd, 1);

	connfd = accept(sockfd, NULL, 0);
	if (!connfd) {
		fprintf(stderr, "server accept failed\n");
		return -1;
	}
		
	freeaddrinfo(res);

	return connfd;
}

int main(int argc, char *argv[])
{
	struct app_context 		*context = NULL;
//	char                    *ib_devname = NULL;
	int						is_client = 0;
	int 					err = 0;
	
	struct app_data	 	 data = {
		.port	    		= 18515,
		.servername 		= NULL,
		.remote_connection 	= NULL,
		.ib_dev     		= NULL
	};

	if(argc == 2){
		data.servername = argv[1];
		is_client = 1;
	}
	if(argc > 2){
		fprintf(stderr, "*Error* Usage: rdma <server>\n");
		return -1;
	}

	pid = getpid();
	srand48(pid * time(NULL));

	err = init_context(&context, &data);
	if (err) {
		fprintf(stderr, "*Error* init_context() failed with %d\n", err);
		return err;		
	}

	err = set_local_ib_connection(context, &data);
	if (err) {
		return err;
	}

	if (!is_client) {
		data.sockfd = tcp_server_listen(&data);
		if (!data.sockfd) {
			fprintf(stderr, "tcp_server_listen (TCP) failed\n");
			return -1;		
		}
	}
	else {
		data.sockfd = tcp_client_connect(&data);
	}
	
	err = tcp_exch_ib_connection_info(&data);
	if (err) {
		fprintf(stderr, "Could not exchange connection, tcp_exch_ib_connection\n");
		return err;
	}
	
	// Print IB-connection details
	print_ib_connection("Local  Connection", &data.local_connection);
	print_ib_connection("Remote Connection", data.remote_connection);	

	err = qp_change_state_rtr(context->qp, &data);
	if (err) {
		fprintf(stderr, "qp modify to rtr failed: %d\n", err);
		return err;
	}

	if (!is_client) {
		err = qp_change_state_rts(context->qp, &data);
		if (err) {
			fprintf(stderr, "qp modify to rts failed: %d\n", err);
			return err;
		}
	}

	// BOTH SIDES READY!
	
	if (!is_client) {
		/* Server - RDMA WRITE */
		printf("Server. Writing to Client-Buffer (RDMA-WRITE)\n");

		// For now, the message to be written into the clients buffer can be edited here
		char *chPtr = context->buf;
		strcpy(chPtr,"MA KORE MOTEK");

		rdma_write(context, &data);
	}
	else
	{
		/* Client - Read local buffer */
		printf("Client. Reading Local-Buffer (Buffer that was registered with MR)\n");
		
		char *chPtr = (char *)data.local_connection.vaddr;
			
		while (1){
			if (strlen(chPtr) > 0){
				break;
			}
		}
		
		printf("Printing local buffer: %s\n" ,chPtr);	
	}
	
	printf("Destroying IB context\n");
	//destroy_ctx(context);
	
	printf("Closing socket\n");	
	close(data.sockfd);
	return 0;
}
