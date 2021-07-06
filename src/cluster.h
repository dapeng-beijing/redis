#ifndef __CLUSTER_H
#define __CLUSTER_H

/*-----------------------------------------------------------------------------
 * Redis cluster data structures, defines, exported API.
 *----------------------------------------------------------------------------*/

#define CLUSTER_SLOTS 16384
#define CLUSTER_OK 0          /* Everything looks ok */
#define CLUSTER_FAIL 1        /* The cluster can't work */
#define CLUSTER_NAMELEN 40    /* sha1 hex length */
#define CLUSTER_PORT_INCR 10000 /* Cluster port = baseport + PORT_INCR */

/* The following defines are amount of time, sometimes expressed as
 * multiplicators of the node timeout value (when ending with MULT). */
#define CLUSTER_DEFAULT_NODE_TIMEOUT 15000
#define CLUSTER_DEFAULT_SLAVE_VALIDITY 10 /* Slave max data age factor. */
#define CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE 1
#define CLUSTER_DEFAULT_SLAVE_NO_FAILOVER 0 /* Failover by default. */
#define CLUSTER_FAIL_REPORT_VALIDITY_MULT 2 /* Fail report validity. */
#define CLUSTER_FAIL_UNDO_TIME_MULT 2 /* Undo fail if master is back. */
#define CLUSTER_FAIL_UNDO_TIME_ADD 10 /* Some additional time. */
#define CLUSTER_FAILOVER_DELAY 5 /* Seconds */
#define CLUSTER_DEFAULT_MIGRATION_BARRIER 1
#define CLUSTER_MF_TIMEOUT 5000 /* Milliseconds to do a manual failover. */
#define CLUSTER_MF_PAUSE_MULT 2 /* Master pause manual failover mult. */
#define CLUSTER_SLAVE_MIGRATION_DELAY 5000 /* Delay for slave migration. */

/* Redirection errors returned by getNodeByQuery(). */
#define CLUSTER_REDIR_NONE 0          /* Node can serve the request. */
#define CLUSTER_REDIR_CROSS_SLOT 1    /* -CROSSSLOT request. */
#define CLUSTER_REDIR_UNSTABLE 2      /* -TRYAGAIN redirection required */
#define CLUSTER_REDIR_ASK 3           /* -ASK redirection required. */
#define CLUSTER_REDIR_MOVED 4         /* -MOVED redirection required. */
#define CLUSTER_REDIR_DOWN_STATE 5    /* -CLUSTERDOWN, global state. */
#define CLUSTER_REDIR_DOWN_UNBOUND 6  /* -CLUSTERDOWN, unbound slot. */

struct clusterNode;

/* clusterLink encapsulates everything needed to talk with a remote node. */
typedef struct clusterLink {
    mstime_t ctime;             /* Link creation time */
    int fd;                     /* TCP socket file descriptor */
    sds sndbuf;                 /* Packet send buffer */
    sds rcvbuf;                 /* Packet reception buffer */
    struct clusterNode *node;   /* Node related to this link if any, or NULL */
} clusterLink;

/* Cluster node flags and macros. */
#define CLUSTER_NODE_MASTER 1     /* The node is a master */
#define CLUSTER_NODE_SLAVE 2      /* The node is a slave */
#define CLUSTER_NODE_PFAIL 4      /* Failure? Need acknowledge */
#define CLUSTER_NODE_FAIL 8       /* The node is believed to be malfunctioning */
#define CLUSTER_NODE_MYSELF 16    /* This node is myself */
#define CLUSTER_NODE_HANDSHAKE 32 /* We have still to exchange the first ping */
#define CLUSTER_NODE_NOADDR   64  /* We don't know the address of this node */
#define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node */
#define CLUSTER_NODE_MIGRATE_TO 256 /* Master elegible for replica migration. */
#define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failver. */
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

#define nodeIsMaster(n) ((n)->flags & CLUSTER_NODE_MASTER)
#define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
#define nodeInHandshake(n) ((n)->flags & CLUSTER_NODE_HANDSHAKE)
#define nodeHasAddr(n) (!((n)->flags & CLUSTER_NODE_NOADDR))
#define nodeWithoutAddr(n) ((n)->flags & CLUSTER_NODE_NOADDR)
#define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
#define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)

/* Reasons why a slave is not able to failover. */
#define CLUSTER_CANT_FAILOVER_NONE 0
#define CLUSTER_CANT_FAILOVER_DATA_AGE 1
#define CLUSTER_CANT_FAILOVER_WAITING_DELAY 2
#define CLUSTER_CANT_FAILOVER_EXPIRED 3
#define CLUSTER_CANT_FAILOVER_WAITING_VOTES 4
#define CLUSTER_CANT_FAILOVER_RELOG_PERIOD (60*5) /* seconds. */

/* clusterState todo_before_sleep flags. */
#define CLUSTER_TODO_HANDLE_FAILOVER (1<<0)
#define CLUSTER_TODO_UPDATE_STATE (1<<1)
#define CLUSTER_TODO_SAVE_CONFIG (1<<2)
#define CLUSTER_TODO_FSYNC_CONFIG (1<<3)

/* Message types.
 *
 * Note that the PING, PONG and MEET messages are actually the same exact
 * kind of packet. PONG is the reply to ping, in the exact format as a PING,
 * while MEET is a special PING that forces the receiver to add the sender
 * as a node (if it is not already in the list). */
#define CLUSTERMSG_TYPE_PING 0          /* Ping */
#define CLUSTERMSG_TYPE_PONG 1          /* Pong (reply to Ping) */
#define CLUSTERMSG_TYPE_MEET 2          /* Meet "let's join" message */
#define CLUSTERMSG_TYPE_FAIL 3          /* Mark node xxx as failing */
#define CLUSTERMSG_TYPE_PUBLISH 4       /* Pub/Sub Publish propagation */
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST 5 /* May I failover? */
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK 6     /* Yes, you have my vote */
#define CLUSTERMSG_TYPE_UPDATE 7        /* Another node slots configuration */
#define CLUSTERMSG_TYPE_MFSTART 8       /* 手动failover包 Pause clients for manual failover */
#define CLUSTERMSG_TYPE_MODULE 9        /* 模块相关包 Module cluster API message. */
#define CLUSTERMSG_TYPE_COUNT 10        /* 计数边界 Total number of message types. */

/* Flags that a module can set in order to prevent certain Redis Cluster
 * features to be enabled. Useful when implementing a different distributed
 * system on top of Redis Cluster message bus, using modules. */
#define CLUSTER_MODULE_FLAG_NONE 0
#define CLUSTER_MODULE_FLAG_NO_FAILOVER (1<<1)
#define CLUSTER_MODULE_FLAG_NO_REDIRECTION (1<<2)

/* This structure represent elements of node->fail_reports. */
typedef struct clusterNodeFailReport {
    struct clusterNode *node;  /* Node reporting the failure condition. */
    mstime_t time;             /* Time of the last report from this node. */
} clusterNodeFailReport;

/* 每个节点都会用一个clusterNode结构记录自己的状态，并为集群内所有其他节点都创建一个clusterNode结构来记录节点状态 */
typedef struct clusterNode {
    mstime_t ctime; /* 节点创建时间 Node object creation time. */
    char name[CLUSTER_NAMELEN]; /* 节点id Node name, hex string, sha1-size */
    int flags;      /* 节点标识：整型，每个bit都代表了不同状态，如节点的主从状态、是否在线、是否在握手等 CLUSTER_NODE_... */
    uint64_t configEpoch; /* 配置纪元：故障转移时起作用，类似于哨兵的配置纪元 Last configEpoch observed for this node */
    unsigned char slots[CLUSTER_SLOTS/8]; /* 槽在该节点中的分布：占用16384/8个字节，16384个比特；每个比特对应一个槽：比特值为1，则该比特对应的槽在节点中；比特值为0，则该比特对应的槽不在节点中 slots handled by this node */
    int numslots;   /* 节点中槽的数量 Number of slots handled by this node */
    int numslaves;  /* Number of slave nodes, if this is a master */
    struct clusterNode **slaves; /* pointers to slave nodes */
    struct clusterNode *slaveof; /* pointer to the master node. Note that it
                                    may be NULL even if the node is a slave
                                    if we don't have the master node in our
                                    tables. */
    mstime_t ping_sent;      /* Unix time we sent latest ping */
    mstime_t pong_received;  /* Unix time we received the pong */
    mstime_t data_received;  /* Unix time we received any data */
    mstime_t fail_time;      /* Unix time when FAIL flag was set */
    mstime_t voted_time;     /* Last time we voted for a slave of this master */
    mstime_t repl_offset_time;  /* Unix time we received offset for this node */
    mstime_t orphaned_time;     /* Starting time of orphaned master condition */
    long long repl_offset;      /* Last known repl offset for this node. */
    char ip[NET_IP_STR_LEN];  /* Latest known IP address of this node */
    int port;                   /* Latest known clients port of this node */
    int cport;                  /* Latest known cluster port of this node. */
    clusterLink *link;          /* TCP/IP link with this node */
    list *fail_reports;         /* List of nodes signaling this as failing */
} clusterNode;

/* clusterState结构保存了在当前节点视角下，集群所处的状态。 */
typedef struct clusterState {
    clusterNode *myself;  /* 自身节点 This node */
    uint64_t currentEpoch;  // 配置纪元
    int state;            /* 集群状态：在线还是下线 CLUSTER_OK, CLUSTER_FAIL, ... */
    int size;             /* 集群中至少包含一个槽的节点数量 Num of master nodes with at least one slot */
    dict *nodes;          /* 哈希表，节点名称->clusterNode节点指针 Hash table of name -> clusterNode structures */
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
    clusterNode *migrating_slots_to[CLUSTER_SLOTS];
    clusterNode *importing_slots_from[CLUSTER_SLOTS];
    clusterNode *slots[CLUSTER_SLOTS];  /* 槽分布信息：数组的每个元素都是一个指向clusterNode结构的指针；如果槽还没有分配给任何节点，则为NULL */
    uint64_t slots_keys_count[CLUSTER_SLOTS];
    rax *slots_to_keys;
    /* The following fields are used to take the slave state on elections. */
    mstime_t failover_auth_time; /* Time of previous or next election. */
    int failover_auth_count;    /* Number of votes received so far. */
    int failover_auth_sent;     /* True if we already asked for votes. */
    int failover_auth_rank;     /* This slave rank for current auth request. */
    uint64_t failover_auth_epoch; /* Epoch of the current election. */
    int cant_failover_reason;   /* Why a slave is currently not able to
                                   failover. See the CANT_FAILOVER_* macros. */
    /* Manual failover state in common. */
    mstime_t mf_end;            /* Manual failover time limit (ms unixtime).
                                   It is zero if there is no MF in progress. */
    /* Manual failover state of master. */
    clusterNode *mf_slave;      /* Slave performing the manual failover. */
    /* Manual failover state of slave. */
    long long mf_master_offset; /* Master offset the slave needs to start MF
                                   or zero if stil not received. */
    int mf_can_start;           /* If non-zero signal that the manual failover
                                   can start requesting masters vote. */
    /* The followign fields are used by masters to take state on elections. */
    uint64_t lastVoteEpoch;     /* Epoch of the last vote granted. */
    int todo_before_sleep; /* Things to do in clusterBeforeSleep(). */
    /* Messages received and sent by type. */
    long long stats_bus_messages_sent[CLUSTERMSG_TYPE_COUNT];
    long long stats_bus_messages_received[CLUSTERMSG_TYPE_COUNT];
    long long stats_pfail_nodes;    /* Number of nodes in PFAIL status,
                                       excluding nodes without address. */
} clusterState;

/* Redis cluster messages header */

/* Initially we don't know our "name", but we'll find it once we connect
 * to the first node, using the getsockname() function. Then we'll use this
 * address for all the next messages. */
/* 从发送节点的视角出发，所记录的关于其他节点的状态信息，包括节点名称、IP地址、状态以及监听地址，等
等。接收方可以据此发现集群中其他的节点或者进行错误发现。 */
typedef struct {
    char nodename[CLUSTER_NAMELEN];     // 节点名称
    uint32_t ping_sent;             // 发送ping的时间
    uint32_t pong_received;         // 接收pong的时间
    char ip[NET_IP_STR_LEN];  /* 节点ip地址 IP address last time it was seen */
    uint16_t port;              /* 节点监听地址 base port last time it was seen */
    uint16_t cport;             /* 节点监听集群通信地址 cluster port last time it was seen */
    uint16_t flags;             /* 节点状态 node->flags copy */
    uint32_t notused1;
} clusterMsgDataGossip;

typedef struct {
    char nodename[CLUSTER_NAMELEN];
} clusterMsgDataFail;

/* 当向集群中任意一个节点发送publish信息后，该节点会向集群中
所有节点广播一条publish包。包中包含有publish信息的渠道信息和消息体信息。 */
typedef struct {
    uint32_t channel_len;       // 渠道名称长度
    uint32_t message_len;       // 消息长度
    unsigned char bulk_data[8]; /* 渠道和消息内容 8 bytes just as placeholder. */
} clusterMsgDataPublish;

/* 当一个节点A发送了一个ping包给B，声明A节点给slot 1000提供
服务，并且ping包中configEpoch为1。接收节点B收到该ping包后，发
现B本地记录的slot 1000是由A1提供服务，并且A1的configEpoch为
2，大于A节点的configEpoch。此时B会向A节点发送一个update包。
包体中会记录A1的配置纪元、节点名称及所提供服务的slot，通知A
更新自身的信息。

 Update包适用于一种特殊情况：当一个主节点M发生故障之后，
其从节点S做了主从切换并且成功升级为主节点，此时S会先将其配
置纪元加1，之后将所有M提供服务的slot更新为由S提供服务。之
后，当M故障恢复进入集群后就会发生上述情况，此时需要向M发送
update包。
 */
typedef struct {
    uint64_t configEpoch; /* 配置纪元 Config epoch of the specified instance. */
    char nodename[CLUSTER_NAMELEN]; /* 节点名称 Name of the slots owner. */
    unsigned char slots[CLUSTER_SLOTS/8]; /* 服务的slot Slots bitmap. */
} clusterMsgDataUpdate;

typedef struct {
    uint64_t module_id;     /* ID of the sender module. */
    uint32_t len;           /* ID of the sender module. */
    uint8_t type;           /* Type from 0 to 255. */
    unsigned char bulk_data[3]; /* 3 bytes just as placeholder. */
} clusterMsgModule;

union clusterMsgData {
    /* PING, MEET and PONG */
    struct {
        /* Array of N clusterMsgDataGossip structures */
        //ping,pong,meet包内容。是个clusterMsgDataGossip类型的数组，根据数组大小使用时确定和分配。该字段称为gossip section
        clusterMsgDataGossip gossip[1];
    } ping;

    /* fail包内容 FAIL */
    struct {
        clusterMsgDataFail about;
    } fail;

    /* publish包内容 PUBLISH */
    struct {
        clusterMsgDataPublish msg;
    } publish;

    /* update包内容 UPDATE */
    struct {
        clusterMsgDataUpdate nodecfg;
    } update;

    /* module包内容 MODULE */
    struct {
        clusterMsgModule msg;
    } module;
};

#define CLUSTER_PROTO_VER 1 /* Cluster bus protocol version. */

/* 集群通信数据包包头 */
typedef struct {
    char sig[4];        /* 固定为RCmb(Redis cluster message bus) Signature "RCmb" (Redis Cluster message bus). */
    uint32_t totlen;    /* 消息总长度 Total length of this message */
    uint16_t ver;       /* 协议版本，当前设值为1 Protocol version, currently set to 1. */
    uint16_t port;      /* 发送方监听的端口 TCP base port number. */
    uint16_t type;      /* 包类型 Message type */
    uint16_t count;     /* data中的gossip section个数(供ping,pong,meet包使 Only used for some kind of messages. */
    uint64_t currentEpoch;  /* 发送方节点记录的集群当前纪元 The epoch accordingly to the sending node. */
    uint64_t configEpoch;   /* 发送方节点对应的配置纪元(如果为从，则为该从所对应的主服务的配置纪元) The config epoch if it's a master, or the last
                               epoch advertised by its master if it is a
                               slave. */
    uint64_t offset;    /* 如果为主，该值表示复制偏移量；如果为从，该值表示从已处理的偏移量 Master replication offset if node is a master or
                           processed replication offset if node is a slave. */
    char sender[CLUSTER_NAMELEN]; /* 发送方名称，40字节 Name of the sender node */
    unsigned char myslots[CLUSTER_SLOTS/8];     /* 发送方提供服务的slot映射表((如果为从，则为该从所对应的主提供服务的slot映射表) */
    char slaveof[CLUSTER_NAMELEN];      /* 发送方如果为从，则该字段为对应的主的名称 */
    char myip[NET_IP_STR_LEN];    /* 发送方IP Sender IP, if not all zeroed. */
    char notused1[34];  /* 预留字段 34 bytes reserved for future usage. */
    uint16_t cport;      /* 发送方监听的cluster bus端口。是集群端口？ 集群端口：端口号是普通端口+10000（10000是固定值，无法改变） Sender TCP cluster bus port */
    uint16_t flags;      /* 发送方节点的flags Sender node flags */
    unsigned char state; /* 发送方节点所记录的集群状态 Cluster state from the POV of the sender */
    unsigned char mflags[3]; /* 目前只有mflags[0]会在手动failover时使用 Message flags: CLUSTERMSG_FLAG[012]_... */
    union clusterMsgData data;  // 包体内容
} clusterMsg;

#define CLUSTERMSG_MIN_LEN (sizeof(clusterMsg)-sizeof(union clusterMsgData))

/* Message flags better specify the packet content or are used to
 * provide some information about the node state. */
#define CLUSTERMSG_FLAG0_PAUSED (1<<0) /* Master paused for manual failover. */
#define CLUSTERMSG_FLAG0_FORCEACK (1<<1) /* Give ACK to AUTH_REQUEST even if
                                            master is up. */

/* ---------------------- API exported outside cluster.c -------------------- */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot, int *ask);
int clusterRedirectBlockedClientIfNeeded(client *c);
void clusterRedirectClient(client *c, clusterNode *n, int hashslot, int error_code);

#endif /* __CLUSTER_H */
