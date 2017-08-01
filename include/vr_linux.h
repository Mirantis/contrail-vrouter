/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */
#ifndef __VR_LINUX_H__
#define __VR_LINUX_H__

#include "vrouter.h"

static inline struct sk_buff *
vp_os_packet(struct vr_packet *pkt)
{
    return CONTAINER_OF(cb, struct sk_buff, pkt);
}

void lh_handle_checksum_complete_skb(struct sk_buff *skb);

#define VROUTER_VERSIONID "1.0"

#endif /* __VR_LINUX_H__ */
