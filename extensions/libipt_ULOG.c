/* Shared library add-on to iptables to add ULOG support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <getopt.h>
#include <iptables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_ULOG.h>

#define ULOG_DEFAULT_NLGROUP 1


void print_groups(unsigned int gmask)
{
	int b;
	unsigned int test;

	for (b = 31; b >= 0; b--)
	{
		test = (1 << b);
		if (gmask & test)
			printf("%d ", b + 1);
	}
}

/* Function which prints out usage message. */
static void help(void)
{
	printf(
"ULOG v%s options:\n"
" --ulog-nlgroup nlgroup		NETLINK grouo used for logging\n"
" --ulog-prefix prefix		Prefix log messages with this prefix.\n\n",
NETFILTER_VERSION);
}

static struct option opts[] = {
	{ "ulog-nlgroup", 1, 0, '!' },
	{ "ulog-prefix", 1, 0, '#' },
	{ 0 }
};

/* Initialize the target. */
static void init(struct ipt_entry_target *t, unsigned int *nfcache)
{
	struct ipt_ulog_info *loginfo = (struct ipt_ulog_info *)t->data;

	loginfo->nl_group = ULOG_DEFAULT_NLGROUP;

	/* Can't cache this */
	*nfcache |= NFC_UNKNOWN;
}

#define IPT_LOG_OPT_NLGROUP 0x01
#define IPT_LOG_OPT_PREFIX 0x02

/* Function which parses command options; returns true if it
   ate an option */
static int parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ipt_entry *entry,
      struct ipt_entry_target **target)
{
	struct ipt_ulog_info *loginfo = (struct ipt_ulog_info *)(*target)->data;
	int group_d;

	switch (c) {
	case '!':
		if (*flags & IPT_LOG_OPT_NLGROUP)
			exit_error(PARAMETER_PROBLEM,
				   "Can't specify --ulog-nlgroup twice");

		if (check_inverse(optarg, &invert))
			exit_error(PARAMETER_PROBLEM,
				   "Unexpected `!' after --ulog-nlgroup");
		group_d = atoi(optarg);
		if (group_d > 32 || group_d < 1)
			exit_error(PARAMETER_PROBLEM,
				"--ulog-nlgroup has to be between 1 and 32");

		loginfo->nl_group = (1 << (group_d -1));

		*flags |= IPT_LOG_OPT_NLGROUP;
		break;

	case '#':
		if (*flags & IPT_LOG_OPT_PREFIX)
			exit_error(PARAMETER_PROBLEM,
				   "Can't specify --ulog-prefix twice");

		if (check_inverse(optarg, &invert))
			exit_error(PARAMETER_PROBLEM,
				   "Unexpected `!' after --ulog-prefix");

		if (strlen(optarg) > sizeof(loginfo->prefix) - 1)
			exit_error(PARAMETER_PROBLEM,
				   "Maximum prefix length %u for --ulog-prefix",
				   sizeof(loginfo->prefix) - 1);

		strcpy(loginfo->prefix, optarg);
		*flags |= IPT_LOG_OPT_PREFIX;
		break;
	}
	return 1;
}

/* Final check; nothing. */
static void final_check(unsigned int flags)
{
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void save(const struct ipt_ip *ip, const struct ipt_entry_target *target)
{
        const struct ipt_ulog_info *loginfo
                = (const struct ipt_ulog_info *)target->data;

        if (strcmp(loginfo->prefix, "") != 0)
                printf("--ulog-prefix %s ", loginfo->prefix);

        if (loginfo->nl_group != ULOG_DEFAULT_NLGROUP)
	{
                printf("--ulog-nlgroup ");
		print_groups(loginfo->nl_group);
		printf("\n");
	}
}

/* Prints out the targinfo. */
static void
print(const struct ipt_ip *ip,
      const struct ipt_entry_target *target,
      int numeric)
{
	const struct ipt_ulog_info *loginfo
		= (const struct ipt_ulog_info *)target->data;

	printf("ULOG ");
	printf("nlgroup ");
	print_groups(loginfo->nl_group);
	if (strcmp(loginfo->prefix, "") != 0)
		printf("prefix `%s' ", loginfo->prefix);
}

struct iptables_target ulog
= { NULL,
    "ULOG",
    NETFILTER_VERSION,
    IPT_ALIGN(sizeof(struct ipt_ulog_info)),
    IPT_ALIGN(sizeof(struct ipt_ulog_info)),
    &help,
    &init,
    &parse,
    &final_check,
    &print,
    &save,
    opts
};

void _init(void)
{
	register_target(&ulog);
}