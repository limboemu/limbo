#ifndef ETHERBOOT_LITTLE_BSWAP_H
#define ETHERBOOT_LITTLE_BSWAP_H

FILE_LICENCE ( GPL2_OR_LATER );

#define htonll(x)	__bswap_64(x)
#define ntohll(x)	__bswap_64(x)
#define ntohl(x)	__bswap_32(x)
#define htonl(x) 	__bswap_32(x)
#define ntohs(x) 	__bswap_16(x)
#define htons(x) 	__bswap_16(x)
#define cpu_to_le64(x)	(x)
#define cpu_to_le32(x)	(x)
#define cpu_to_le16(x)	(x)
#define cpu_to_be64(x)	__bswap_64(x)
#define cpu_to_be32(x)	__bswap_32(x)
#define cpu_to_be16(x)	__bswap_16(x)
#define le64_to_cpu(x)	(x)
#define le32_to_cpu(x)	(x)
#define le16_to_cpu(x)	(x)
#define be64_to_cpu(x)	__bswap_64(x)
#define be32_to_cpu(x)	__bswap_32(x)
#define be16_to_cpu(x)	__bswap_16(x)
#define cpu_to_le64s(x) do {} while (0)
#define cpu_to_le32s(x) do {} while (0)
#define cpu_to_le16s(x) do {} while (0)
#define cpu_to_be64s(x) __bswap_64s(x)
#define cpu_to_be32s(x) __bswap_32s(x)
#define cpu_to_be16s(x) __bswap_16s(x)
#define le64_to_cpus(x) do {} while (0)
#define le32_to_cpus(x) do {} while (0)
#define le16_to_cpus(x) do {} while (0)
#define be64_to_cpus(x) __bswap_64s(x)
#define be32_to_cpus(x) __bswap_32s(x)
#define be16_to_cpus(x) __bswap_16s(x)

#endif /* ETHERBOOT_LITTLE_BSWAP_H */
