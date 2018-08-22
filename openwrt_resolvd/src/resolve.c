
#include "resolv.h"

const char *resolv_strerror(int resolv_errno)
{
	switch(resolv_errno)
	{
	case RESOLV_ERR_FAIL:
		return "Connection pending.";
	case RESOLV_ERR_SUCCESS:
		return "No error and success.";
	case RESOLV_ERR_NOCARD:
		return "Network card has not been found.";
	case RESOLV_ERR_IFDOWN:
		return "Network card is stopped.";
	case RESOLV_ERR_MACZERO:
		return "MAC addresses are not burn.";
	case RESOLV_ERR_UNASSOCIATED:
		return "Host is not connected to the router.";
	case RESOLV_ERR_UNCONFIGURED:
		return "The network is not configured.";
	case RESOLV_ERR_WITHOUT_IP:
		return "Message not found (internal error).";
	case RESOLV_ERR_DIE_UDHCPC:
		return "DHCP client is not started.";
	case RESOLV_ERR_PUB_NET_REFUSE:
		return "Host does not have access to the public Internet.";
	case RESOLV_ERR_DNS_FAIL:
		return "DNS configuration errors.";
	case RESOLV_ERR_FIREWALL_BLOCKED:
		return "The service blocked by the firewall.";
	default:
		return "Unknown error.";
	}
}

