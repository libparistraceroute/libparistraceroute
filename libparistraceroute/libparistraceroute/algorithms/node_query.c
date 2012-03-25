//struct traceroute_node_query_t {
//    // Inputs
//    ip_t        target_ip;
//    int32       ttl;
//    protocol_t  protocol;
//
//    // Outputs
//
//    // Callbacks (called when the algorithm terminates for instance)
//    void (*) on_icmp_error(void *pdata, int icmp_num_error);
//    void (*) on_discovered_node(void *pdata, ip_t ip, flow_t flow, int ttl, timestamp_t ts);
//}
//
//traceroute_node_query_init(
//    algorithm_t             * algorithm,
//    traceroute_node_query_t * options,    // Pass NULL if no options
//);
//
//traceroute_node_query_init(
//    algorithm_t             * algorithm,
//    traceroute_node_query_t * options,    // Pass NULL if no options
//) {
//    if(options) {
//        //                       Probe field(s)  Op°     Options field
//        add_predicate(algorithm, TARGET_IP,      EQUAL,  options->target_ip);
//        add_predicate(algorithm, TTL,            EQUAL,  options->ttl);
//        add_predicate(algorithm, PROTOCOL,       EQUAL,  options->protocol);
//    }
//
//    // Init the algorithm...
//}
//
//traceroute_node_query_t traceroute_node_query_default = {
//    .protocol = UDP;
//}
//
//int traceroute_node_query_check(
//    algorithm_t             * algorithm,
//    traceroute_node_query_t * options
//){
//    if (options->ttl <= 0) return 0;
//    //...
//    return 1;
//}
//
//static algorithm_ops_t node_query = {
//    .next=NULL,
//    .name="node_query",
//    .personal_fields = personal_fields_node_query,
//    .nb_field = FIELDS_MDA,
//    .init=node_query_init,
//    .on_reply=node_query_on_reply,
//    /*
//    .terminate = node_query_terminate,
//    */
//};
//
//REGISTER_ALGORITHM(node_query);
//
