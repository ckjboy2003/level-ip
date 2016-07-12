#include "syshead.h"
#include "utils.h"
#include "netdev.h"
#include "ethernet.h"
#include "tuntap_if.h"
#include "basic.h"

struct netdev netdev;

void netdev_init(char *addr, char *hwaddr)
{
    struct netdev *dev = &netdev;
    CLEAR(*dev);

    dev->buflen = 512;

    if (inet_pton(AF_INET, addr, &dev->addr) != 1) {
        perror("ERR: Parsing inet address failed\n");
        exit(1);
    }

    sscanf(hwaddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dev->hwaddr[0],
                                                    &dev->hwaddr[1],
                                                    &dev->hwaddr[2],
                                                    &dev->hwaddr[3],
                                                    &dev->hwaddr[4],
                                                    &dev->hwaddr[5]);

    dev->tundev = calloc(10, 1);

    tun_init(dev->tundev);
}

void netdev_transmit(struct netdev *dev, struct eth_hdr *hdr, 
                     uint16_t ethertype, int len, uint8_t *dst)
{
    uint8_t dst_mac[6];
    memcpy(dst_mac, dst, 6);
    hdr->ethertype = htons(ethertype);

    memcpy(hdr->smac, dev->hwaddr, 6);
    memcpy(hdr->dmac, dst_mac, 6);

    len += sizeof(struct eth_hdr);

    tun_write((char *)hdr, len);
}

void *netdev_rx_loop()
{
    int running = 1;
    
    while (running) {
        if (tun_read(netdev.buf, netdev.buflen) < 0) {
            print_error("ERR: Read from tun_fd: %s\n", strerror(errno));
            return 1;
        }

        struct eth_hdr *hdr = init_eth_hdr(netdev.buf);

        handle_frame(&netdev, hdr);
    }
}
