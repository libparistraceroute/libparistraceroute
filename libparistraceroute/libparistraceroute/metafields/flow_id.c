static metafield_t flow_id =
{
    .name     = "flow_id",
    .patterns = {
        "IPv4<src_ip,dst_ip,proto>/UDP<src_port,dst_port>",
    }
}

METAFIELD_REGISTER(flow_id);
