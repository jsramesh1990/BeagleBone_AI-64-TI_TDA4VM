/*
 * ============================================================
 * BeagleBone AI-64 - TI TDA4VM
 * Packet Filter Project
 *
 * Performance Benchmark
 *
 * File:
 *     tests/performance/benchmark.c
 *
 * Purpose:
 *     Benchmark packet-filter performance from userspace.
 *
 * Measurements:
 *     - IOCTL latency
 *     - Packet processing throughput
 *     - Rule lookup performance
 *     - Statistics access latency
 *     - Sustained operation
 *     - CPU time
 *     - Wall-clock execution time
 *
 * Usage:
 *
 *     ./benchmark
 *
 *     ./benchmark --iterations 100000
 *
 *     ./benchmark --device /dev/packet_filter
 *
 *     ./benchmark --iterations 100000 --device /dev/packet_filter
 *
 * ============================================================
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * ------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------
 */

#define DEFAULT_DEVICE       "/dev/packet_filter"
#define DEFAULT_ITERATIONS   100000ULL
#define DEFAULT_WARMUP       1000ULL

#define MAX_PACKET_SIZE      2048
#define MAX_RULES            1024

#define NS_PER_SEC            1000000000ULL

/*
 * ------------------------------------------------------------
 * IOCTL Definitions
 *
 * Keep these definitions synchronized with:
 *
 *     kernel/packet_filter/ioctl_defs.h
 *
 * If ioctl_defs.h is available during the userspace build,
 * it should preferably be included directly.
 * ------------------------------------------------------------
 */

#ifndef PACKET_FILTER_IOCTL_MAGIC

#define PACKET_FILTER_IOCTL_MAGIC 'P'

#define PACKET_FILTER_IOCTL_GET_STATUS \
    _IOR(PACKET_FILTER_IOCTL_MAGIC, 0x01, uint32_t)

#define PACKET_FILTER_IOCTL_GET_STATS \
    _IOR(PACKET_FILTER_IOCTL_MAGIC, 0x02, uint64_t)

#define PACKET_FILTER_IOCTL_ENABLE \
    _IO(PACKET_FILTER_IOCTL_MAGIC, 0x03)

#define PACKET_FILTER_IOCTL_DISABLE \
    _IO(PACKET_FILTER_IOCTL_MAGIC, 0x04)

#define PACKET_FILTER_IOCTL_CLEAR_STATS \
    _IO(PACKET_FILTER_IOCTL_MAGIC, 0x05)

#endif

/*
 * ------------------------------------------------------------
 * Statistics Structure
 * ------------------------------------------------------------
 */

struct benchmark_stats {
    uint64_t accepted;
    uint64_t dropped;
    uint64_t monitored;
};

/*
 * ------------------------------------------------------------
 * Benchmark Configuration
 * ------------------------------------------------------------
 */

struct benchmark_config {
    const char *device;

    uint64_t iterations;

    uint64_t warmup;

    int verbose;

    int ioctl_test;

    int throughput_test;

    int statistics_test;
};

/*
 * ------------------------------------------------------------
 * Benchmark Result
 * ------------------------------------------------------------
 */

struct benchmark_result {
    uint64_t operations;

    uint64_t elapsed_ns;

    double operations_per_second;

    double average_ns;

    double minimum_ns;

    double maximum_ns;

    double cpu_time_ns;
};

/*
 * ------------------------------------------------------------
 * Time Helpers
 * ------------------------------------------------------------
 */

static uint64_t get_time_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * NS_PER_SEC) +
           (uint64_t)ts.tv_nsec;
}


/*
 * ------------------------------------------------------------
 * CPU Time
 * ------------------------------------------------------------
 */

static uint64_t get_cpu_time_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * NS_PER_SEC) +
           (uint64_t)ts.tv_nsec;
}


/*
 * ------------------------------------------------------------
 * Sleep Helper
 * ------------------------------------------------------------
 */

static void short_delay(void)
{
    struct timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = 1000000L;

    nanosleep(&ts, NULL);
}


/*
 * ------------------------------------------------------------
 * Error Helper
 * ------------------------------------------------------------
 */

static void print_errno(const char *operation)
{
    fprintf(stderr,
            "[ERROR] %s: %s\n",
            operation,
            strerror(errno));
}


/*
 * ------------------------------------------------------------
 * Open Packet Filter Device
 * ------------------------------------------------------------
 */

static int open_packet_filter(const char *device)
{
    int fd;

    fd = open(device, O_RDWR | O_CLOEXEC);

    if (fd < 0) {
        print_errno("open packet-filter device");
        return -1;
    }

    return fd;
}


/*
 * ------------------------------------------------------------
 * Close Device
 * ------------------------------------------------------------
 */

static void close_packet_filter(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}


/*
 * ------------------------------------------------------------
 * IOCTL GET STATUS
 * ------------------------------------------------------------
 */

static int ioctl_get_status(int fd, uint32_t *status)
{
    if (status == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (ioctl(fd,
              PACKET_FILTER_IOCTL_GET_STATUS,
              status) < 0) {
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * IOCTL GET STATS
 * ------------------------------------------------------------
 */

static int ioctl_get_stats(int fd,
                           struct benchmark_stats *stats)
{
    if (stats == NULL) {
        errno = EINVAL;
        return -1;
    }

    /*
     * The exact userspace structure must match
     * kernel/packet_filter/ioctl_defs.h.
     *
     * This fallback benchmark assumes the driver returns
     * a statistics structure with accepted/dropped/monitored
     * counters.
     */

    if (ioctl(fd,
              PACKET_FILTER_IOCTL_GET_STATS,
              stats) < 0) {
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * IOCTL ENABLE
 * ------------------------------------------------------------
 */

static int ioctl_enable(int fd)
{
    if (ioctl(fd,
              PACKET_FILTER_IOCTL_ENABLE,
              NULL) < 0) {
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * IOCTL DISABLE
 * ------------------------------------------------------------
 */

static int ioctl_disable(int fd)
{
    if (ioctl(fd,
              PACKET_FILTER_IOCTL_DISABLE,
              NULL) < 0) {
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * IOCTL CLEAR STATS
 * ------------------------------------------------------------
 */

static int ioctl_clear_stats(int fd)
{
    if (ioctl(fd,
              PACKET_FILTER_IOCTL_CLEAR_STATS,
              NULL) < 0) {
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * Print Header
 * ------------------------------------------------------------
 */

static void print_header(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("       BeagleBone AI-64 - TI TDA4VM\n");
    printf("       Packet Filter Performance Benchmark\n");
    printf("============================================================\n");
    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Print Configuration
 * ------------------------------------------------------------
 */

static void print_configuration(
        const struct benchmark_config *config)
{
    printf("Benchmark Configuration\n");
    printf("-----------------------\n");

    printf("Device       : %s\n",
           config->device);

    printf("Iterations   : %" PRIu64 "\n",
           config->iterations);

    printf("Warmup       : %" PRIu64 "\n",
           config->warmup);

    printf("IOCTL test   : %s\n",
           config->ioctl_test ? "enabled" : "disabled");

    printf("Throughput   : %s\n",
           config->throughput_test ? "enabled" : "disabled");

    printf("Statistics   : %s\n",
           config->statistics_test ? "enabled" : "disabled");

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Print Result
 * ------------------------------------------------------------
 */

static void print_result(
        const char *name,
        const struct benchmark_result *result)
{
    printf("\n");
    printf("------------------------------------------------------------\n");
    printf("%s\n", name);
    printf("------------------------------------------------------------\n");

    printf("Operations          : %" PRIu64 "\n",
           result->operations);

    printf("Elapsed time        : %" PRIu64 " ns\n",
           result->elapsed_ns);

    printf("Average latency     : %.2f ns\n",
           result->average_ns);

    printf("Minimum latency     : %.2f ns\n",
           result->minimum_ns);

    printf("Maximum latency     : %.2f ns\n",
           result->maximum_ns);

    printf("Operations/sec      : %.2f\n",
           result->operations_per_second);

    printf("CPU time            : %.2f ms\n",
           result->cpu_time_ns / 1000000.0);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Initialize Result
 * ------------------------------------------------------------
 */

static void initialize_result(
        struct benchmark_result *result)
{
    memset(result, 0, sizeof(*result));

    result->minimum_ns = 0.0;

    result->maximum_ns = 0.0;
}


/*
 * ------------------------------------------------------------
 * Finalize Result
 * ------------------------------------------------------------
 */

static void finalize_result(
        struct benchmark_result *result,
        uint64_t start_ns,
        uint64_t end_ns,
        uint64_t cpu_start_ns,
        uint64_t cpu_end_ns)
{
    result->elapsed_ns = end_ns - start_ns;

    result->cpu_time_ns =
        cpu_end_ns - cpu_start_ns;

    if (result->operations == 0) {
        return;
    }

    result->average_ns =
        (double)result->elapsed_ns /
        (double)result->operations;

    if (result->elapsed_ns > 0) {

        result->operations_per_second =
            ((double)result->operations *
             (double)NS_PER_SEC) /
            (double)result->elapsed_ns;
    }
}


/*
 * ------------------------------------------------------------
 * Warmup
 * ------------------------------------------------------------
 */

static int benchmark_warmup(
        int fd,
        uint64_t iterations)
{
    uint32_t status;

    uint64_t i;

    if (iterations == 0) {
        return 0;
    }

    printf("[INFO] Running %" PRIu64
           " warmup operations...\n",
           iterations);

    for (i = 0; i < iterations; i++) {

        if (ioctl_get_status(fd, &status) < 0) {

            print_errno("warmup GET_STATUS");

            return -1;
        }
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * IOCTL Latency Benchmark
 * ------------------------------------------------------------
 */

static int benchmark_ioctl_latency(
        int fd,
        const struct benchmark_config *config,
        struct benchmark_result *result)
{
    uint64_t start_ns;
    uint64_t end_ns;

    uint64_t cpu_start_ns;
    uint64_t cpu_end_ns;

    uint64_t i;

    uint32_t status;

    initialize_result(result);

    printf("[INFO] Benchmarking GET_STATUS IOCTL...\n");

    start_ns = get_time_ns();

    cpu_start_ns = get_cpu_time_ns();

    for (i = 0; i < config->iterations; i++) {

        uint64_t operation_start;
        uint64_t operation_end;
        double latency;

        operation_start = get_time_ns();

        if (ioctl_get_status(fd, &status) < 0) {

            print_errno("GET_STATUS IOCTL");

            return -1;
        }

        operation_end = get_time_ns();

        latency =
            (double)(operation_end -
                     operation_start);

        if (result->minimum_ns == 0.0 ||
            latency < result->minimum_ns) {

            result->minimum_ns = latency;
        }

        if (latency > result->maximum_ns) {

            result->maximum_ns = latency;
        }

        result->operations++;

        if (config->verbose &&
            ((i % 10000ULL) == 0)) {

            printf("  operation: %" PRIu64 "\n",
                   i);
        }
    }

    end_ns = get_time_ns();

    cpu_end_ns = get_cpu_time_ns();

    finalize_result(result,
                    start_ns,
                    end_ns,
                    cpu_start_ns,
                    cpu_end_ns);

    return 0;
}


/*
 * ------------------------------------------------------------
 * Statistics IOCTL Benchmark
 * ------------------------------------------------------------
 */

static int benchmark_statistics(
        int fd,
        const struct benchmark_config *config,
        struct benchmark_result *result)
{
    uint64_t start_ns;
    uint64_t end_ns;

    uint64_t cpu_start_ns;
    uint64_t cpu_end_ns;

    uint64_t i;

    initialize_result(result);

    printf("[INFO] Benchmarking GET_STATS IOCTL...\n");

    start_ns = get_time_ns();

    cpu_start_ns = get_cpu_time_ns();

    for (i = 0; i < config->iterations; i++) {

        struct benchmark_stats stats;

        uint64_t operation_start;
        uint64_t operation_end;

        double latency;

        memset(&stats, 0, sizeof(stats));

        operation_start = get_time_ns();

        if (ioctl_get_stats(fd, &stats) < 0) {

            print_errno("GET_STATS IOCTL");

            return -1;
        }

        operation_end = get_time_ns();

        latency =
            (double)(operation_end -
                     operation_start);

        if (result->minimum_ns == 0.0 ||
            latency < result->minimum_ns) {

            result->minimum_ns = latency;
        }

        if (latency > result->maximum_ns) {

            result->maximum_ns = latency;
        }

        result->operations++;
    }

    end_ns = get_time_ns();

    cpu_end_ns = get_cpu_time_ns();

    finalize_result(result,
                    start_ns,
                    end_ns,
                    cpu_start_ns,
                    cpu_end_ns);

    return 0;
}


/*
 * ------------------------------------------------------------
 * Print Packet Statistics
 * ------------------------------------------------------------
 */

static void print_packet_statistics(
        int fd)
{
    struct benchmark_stats stats;

    memset(&stats, 0, sizeof(stats));

    if (ioctl_get_stats(fd, &stats) < 0) {

        print_errno("GET_STATS");

        return;
    }

    printf("\n");
    printf("Packet Filter Statistics\n");
    printf("------------------------\n");

    printf("Accepted packets : %" PRIu64 "\n",
           stats.accepted);

    printf("Dropped packets  : %" PRIu64 "\n",
           stats.dropped);

    printf("Monitored packets: %" PRIu64 "\n",
           stats.monitored);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Enable Driver
 * ------------------------------------------------------------
 */

static void enable_driver(int fd)
{
    if (ioctl_enable(fd) < 0) {

        /*
         * The exact driver may not implement ENABLE.
         *
         * Do not terminate the benchmark here.
         */

        if (errno == ENOTTY ||
            errno == EINVAL) {

            printf("[INFO] ENABLE IOCTL not available.\n");

            return;
        }

        print_errno("ENABLE IOCTL");

        return;
    }

    printf("[INFO] Packet filter enabled.\n");
}


/*
 * ------------------------------------------------------------
 * Disable Driver
 * ------------------------------------------------------------
 */

static void disable_driver(int fd)
{
    if (ioctl_disable(fd) < 0) {

        if (errno == ENOTTY ||
            errno == EINVAL) {

            printf("[INFO] DISABLE IOCTL not available.\n");

            return;
        }

        print_errno("DISABLE IOCTL");

        return;
    }

    printf("[INFO] Packet filter disabled.\n");
}


/*
 * ------------------------------------------------------------
 * Clear Statistics
 * ------------------------------------------------------------
 */

static void clear_statistics(int fd)
{
    if (ioctl_clear_stats(fd) < 0) {

        if (errno == ENOTTY ||
            errno == EINVAL) {

            printf("[INFO] CLEAR_STATS IOCTL not available.\n");

            return;
        }

        print_errno("CLEAR_STATS IOCTL");

        return;
    }

    printf("[INFO] Packet statistics cleared.\n");
}


/*
 * ------------------------------------------------------------
 * Driver Status
 * ------------------------------------------------------------
 */

static void print_driver_status(int fd)
{
    uint32_t status = 0;

    if (ioctl_get_status(fd, &status) < 0) {

        print_errno("GET_STATUS");

        return;
    }

    printf("\n");
    printf("Packet Filter Status\n");
    printf("--------------------\n");

    printf("Raw status: 0x%08" PRIx32 "\n",
           status);

    printf("Decimal   : %" PRIu32 "\n",
           status);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Throughput Calculation
 * ------------------------------------------------------------
 */

static double calculate_mpps(
        uint64_t packets,
        uint64_t elapsed_ns)
{
    if (elapsed_ns == 0) {
        return 0.0;
    }

    return ((double)packets /
            (double)elapsed_ns) *
           1000.0;
}


/*
 * ------------------------------------------------------------
 * Software Packet Processing Benchmark
 *
 * This does NOT transmit packets onto Ethernet.
 *
 * It measures the cost of userspace packet-buffer
 * preparation and memory access.
 * ------------------------------------------------------------
 */

static int benchmark_packet_buffer(
        const struct benchmark_config *config,
        struct benchmark_result *result)
{
    uint8_t *packet;

    uint64_t start_ns;
    uint64_t end_ns;

    uint64_t cpu_start_ns;
    uint64_t cpu_end_ns;

    uint64_t i;

    volatile uint64_t checksum = 0;

    initialize_result(result);

    packet = malloc(MAX_PACKET_SIZE);

    if (packet == NULL) {

        fprintf(stderr,
                "[ERROR] Unable to allocate packet buffer\n");

        return -1;
    }

    memset(packet, 0xAA, MAX_PACKET_SIZE);

    printf("[INFO] Running packet-buffer benchmark...\n");

    start_ns = get_time_ns();

    cpu_start_ns = get_cpu_time_ns();

    for (i = 0; i < config->iterations; i++) {

        size_t offset;

        for (offset = 0;
             offset < MAX_PACKET_SIZE;
             offset += 64) {

            checksum += packet[offset];
        }

        result->operations++;
    }

    end_ns = get_time_ns();

    cpu_end_ns = get_cpu_time_ns();

    finalize_result(result,
                    start_ns,
                    end_ns,
                    cpu_start_ns,
                    cpu_end_ns);

    free(packet);

    /*
     * Prevent compiler optimization.
     */

    if (checksum == UINT64_MAX) {
        printf("Checksum: %" PRIu64 "\n",
               checksum);
    }

    /*
     * Demonstrate packets/sec -> Mpps.
     */

    printf("Packet-buffer throughput: %.3f Mpps\n",
           calculate_mpps(
               result->operations,
               result->elapsed_ns));

    return 0;
}


/*
 * ------------------------------------------------------------
 * System Information
 * ------------------------------------------------------------
 */

static void print_system_information(void)
{
    struct utsname {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
    };

    FILE *fp;

    char buffer[256];

    printf("System Information\n");
    printf("------------------\n");

    printf("Architecture: ");

    fp = fopen("/proc/sys/kernel/arch", "r");

    if (fp != NULL) {

        if (fgets(buffer,
                  sizeof(buffer),
                  fp) != NULL) {

            printf("%s", buffer);

        }

        fclose(fp);

    } else {

        printf("unknown\n");
    }

    printf("Kernel      : ");

    fp = fopen("/proc/sys/kernel/osrelease", "r");

    if (fp != NULL) {

        if (fgets(buffer,
                  sizeof(buffer),
                  fp) != NULL) {

            printf("%s", buffer);

        }

        fclose(fp);

    } else {

        printf("unknown\n");
    }

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * CPU Information
 * ------------------------------------------------------------
 */

static void print_cpu_information(void)
{
    FILE *fp;

    char line[512];

    printf("CPU Information\n");
    printf("----------------\n");

    fp = fopen("/proc/cpuinfo", "r");

    if (fp == NULL) {

        printf("Unable to read /proc/cpuinfo\n");

        return;
    }

    while (fgets(line,
                 sizeof(line),
                 fp) != NULL) {

        if (strncmp(line,
                    "model name",
                    10) == 0 ||

            strncmp(line,
                    "CPU architecture",
                    16) == 0 ||

            strncmp(line,
                    "CPU implementer",
                    15) == 0 ||

            strncmp(line,
                    "CPU part",
                    8) == 0) {

            printf("%s", line);
        }
    }

    fclose(fp);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Memory Information
 * ------------------------------------------------------------
 */

static void print_memory_information(void)
{
    FILE *fp;

    char line[512];

    printf("Memory Information\n");
    printf("------------------\n");

    fp = fopen("/proc/meminfo", "r");

    if (fp == NULL) {

        printf("Unable to read /proc/meminfo\n");

        return;
    }

    while (fgets(line,
                 sizeof(line),
                 fp) != NULL) {

        if (strncmp(line,
                    "MemTotal:",
                    9) == 0 ||

            strncmp(line,
                    "MemAvailable:",
                    13) == 0) {

            printf("%s", line);
        }
    }

    fclose(fp);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Resource Usage
 * ------------------------------------------------------------
 */

static void print_resource_usage(void)
{
    struct rusage usage;

    if (getrusage(RUSAGE_SELF,
                  &usage) != 0) {

        print_errno("getrusage");

        return;
    }

    printf("Process Resource Usage\n");
    printf("----------------------\n");

    printf("User CPU time   : %ld.%06ld sec\n",
           usage.ru_utime.tv_sec,
           usage.ru_utime.tv_usec);

    printf("System CPU time : %ld.%06ld sec\n",
           usage.ru_stime.tv_sec,
           usage.ru_stime.tv_usec);

    printf("Max RSS         : %ld KB\n",
           usage.ru_maxrss);

    printf("Minor faults    : %ld\n",
           usage.ru_minflt);

    printf("Major faults    : %ld\n",
           usage.ru_majflt);

    printf("Voluntary ctx   : %ld\n",
           usage.ru_nvcsw);

    printf("Involuntary ctx : %ld\n",
           usage.ru_nivcsw);

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Parse Unsigned 64-bit
 * ------------------------------------------------------------
 */

static int parse_uint64(
        const char *value,
        uint64_t *result)
{
    char *end = NULL;

    unsigned long long number;

    if (value == NULL ||
        result == NULL) {

        return -1;
    }

    errno = 0;

    number = strtoull(value,
                      &end,
                      10);

    if (errno != 0 ||
        end == value ||
        *end != '\0') {

        return -1;
    }

    *result = (uint64_t)number;

    return 0;
}


/*
 * ------------------------------------------------------------
 * Command-Line Help
 * ------------------------------------------------------------
 */

static void print_usage(
        const char *program)
{
    printf("\n");
    printf("Usage:\n");
    printf("\n");

    printf("  %s [options]\n",
           program);

    printf("\n");

    printf("Options:\n");

    printf("  --device <path>\n");
    printf("      Packet-filter device.\n");
    printf("      Default: %s\n",
           DEFAULT_DEVICE);

    printf("\n");

    printf("  --iterations <count>\n");
    printf("      Number of benchmark operations.\n");
    printf("      Default: %" PRIu64 "\n",
           DEFAULT_ITERATIONS);

    printf("\n");

    printf("  --warmup <count>\n");
    printf("      Number of warmup operations.\n");
    printf("      Default: %" PRIu64 "\n",
           DEFAULT_WARMUP);

    printf("\n");

    printf("  --no-ioctl\n");
    printf("      Disable IOCTL benchmark.\n");

    printf("\n");

    printf("  --no-throughput\n");
    printf("      Disable packet-buffer benchmark.\n");

    printf("\n");

    printf("  --no-stats\n");
    printf("      Disable statistics benchmark.\n");

    printf("\n");

    printf("  --verbose\n");
    printf("      Enable verbose output.\n");

    printf("\n");

    printf("  --help\n");
    printf("      Display this help.\n");

    printf("\n");
}


/*
 * ------------------------------------------------------------
 * Parse Arguments
 * ------------------------------------------------------------
 */

static int parse_arguments(
        int argc,
        char **argv,
        struct benchmark_config *config)
{
    int i;

    config->device = DEFAULT_DEVICE;

    config->iterations =
        DEFAULT_ITERATIONS;

    config->warmup =
        DEFAULT_WARMUP;

    config->verbose = 0;

    config->ioctl_test = 1;

    config->throughput_test = 1;

    config->statistics_test = 1;

    for (i = 1; i < argc; i++) {

        if (strcmp(argv[i],
                   "--device") == 0) {

            if ((i + 1) >= argc) {

                fprintf(stderr,
                        "--device requires a path\n");

                return -1;
            }

            config->device =
                argv[++i];

        } else if (strcmp(argv[i],
                          "--iterations") == 0) {

            if ((i + 1) >= argc) {

                fprintf(stderr,
                        "--iterations requires a value\n");

                return -1;
            }

            if (parse_uint64(
                    argv[++i],
                    &config->iterations) != 0) {

                fprintf(stderr,
                        "Invalid iteration count\n");

                return -1;
            }

        } else if (strcmp(argv[i],
                          "--warmup") == 0) {

            if ((i + 1) >= argc) {

                fprintf(stderr,
                        "--warmup requires a value\n");

                return -1;
            }

            if (parse_uint64(
                    argv[++i],
                    &config->warmup) != 0) {

                fprintf(stderr,
                        "Invalid warmup count\n");

                return -1;
            }

        } else if (strcmp(argv[i],
                          "--no-ioctl") == 0) {

            config->ioctl_test = 0;

        } else if (strcmp(argv[i],
                          "--no-throughput") == 0) {

            config->throughput_test = 0;

        } else if (strcmp(argv[i],
                          "--no-stats") == 0) {

            config->statistics_test = 0;

        } else if (strcmp(argv[i],
                          "--verbose") == 0) {

            config->verbose = 1;

        } else if (strcmp(argv[i],
                          "--help") == 0 ||
                   strcmp(argv[i],
                          "-h") == 0) {

            print_usage(argv[0]);

            exit(EXIT_SUCCESS);

        } else {

            fprintf(stderr,
                    "Unknown option: %s\n",
                    argv[i]);

            return -1;
        }
    }

    if (config->iterations == 0) {

        fprintf(stderr,
                "Iterations must be greater than zero\n");

        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------
 * Print Final Summary
 * ------------------------------------------------------------
 */

static void print_summary(
        const struct benchmark_result *ioctl_result,
        const struct benchmark_result *stats_result,
        const struct benchmark_result *throughput_result)
{
    printf("\n");
    printf("============================================================\n");
    printf("                  BENCHMARK SUMMARY\n");
    printf("============================================================\n");

    if (ioctl_result != NULL) {

        printf("\n");
        printf("IOCTL GET_STATUS\n");

        printf("  Operations/sec : %.2f\n",
               ioctl_result->operations_per_second);

        printf("  Average        : %.2f ns\n",
               ioctl_result->average_ns);

        printf("  Minimum        : %.2f ns\n",
               ioctl_result->minimum_ns);

        printf("  Maximum        : %.2f ns\n",
               ioctl_result->maximum_ns);
    }

    if (stats_result != NULL) {

        printf("\n");
        printf("IOCTL GET_STATS\n");

        printf("  Operations/sec : %.2f\n",
               stats_result->operations_per_second);

        printf("  Average        : %.2f ns\n",
               stats_result->average_ns);

        printf("  Minimum        : %.2f ns\n",
               stats_result->minimum_ns);

        printf("  Maximum        : %.2f ns\n",
               stats_result->maximum_ns);
    }

    if (throughput_result != NULL) {

        printf("\n");
        printf("Packet Buffer\n");

        printf("  Operations/sec : %.2f\n",
               throughput_result->operations_per_second);

        printf("  Throughput     : %.3f Mpps\n",
               calculate_mpps(
                   throughput_result->operations,
                   throughput_result->elapsed_ns));
    }

    printf("\n");
    printf("============================================================\n");
}


/*
 * ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------
 */

int main(int argc, char **argv)
{
    struct benchmark_config config;

    struct benchmark_result ioctl_result;
    struct benchmark_result stats_result;
    struct benchmark_result throughput_result;

    struct benchmark_result *ioctl_result_ptr = NULL;
    struct benchmark_result *stats_result_ptr = NULL;
    struct benchmark_result *throughput_result_ptr = NULL;

    int fd = -1;

    int ret;

    print_header();

    /*
     * --------------------------------------------------------
     * Parse arguments
     * --------------------------------------------------------
     */

    ret = parse_arguments(argc,
                          argv,
                          &config);

    if (ret != 0) {

        print_usage(argv[0]);

        return EXIT_FAILURE;
    }

    /*
     * --------------------------------------------------------
     * Configuration
     * --------------------------------------------------------
     */

    print_configuration(&config);

    /*
     * --------------------------------------------------------
     * System information
     * --------------------------------------------------------
     */

    print_system_information();

    print_cpu_information();

    print_memory_information();

    /*
     * --------------------------------------------------------
     * Open driver
     * --------------------------------------------------------
     */

    if (config.ioctl_test ||
        config.statistics_test) {

        printf("[INFO] Opening packet-filter device:\n");
        printf("       %s\n",
               config.device);

        fd = open_packet_filter(
                config.device);

        if (fd < 0) {

            fprintf(stderr,
                    "\n[ERROR] Unable to open packet-filter device.\n");

            fprintf(stderr,
                    "Make sure the kernel driver is loaded:\n");

            fprintf(stderr,
                    "    sudo modprobe packet_filter\n");

            fprintf(stderr,
                    "\nOr specify another device:\n");

            fprintf(stderr,
                    "    --device /dev/packet_filter\n");

            return EXIT_FAILURE;
        }

        printf("[PASS] Packet-filter device opened.\n");

        /*
         * ----------------------------------------------------
         * Driver status
         * ----------------------------------------------------
         */

        print_driver_status(fd);

        /*
         * ----------------------------------------------------
         * Clear previous statistics
         * ----------------------------------------------------
         */

        clear_statistics(fd);

        /*
         * ----------------------------------------------------
         * Enable packet filter
         * ----------------------------------------------------
         */

        enable_driver(fd);

        /*
         * ----------------------------------------------------
         * Warmup
         * ----------------------------------------------------
         */

        if (benchmark_warmup(
                fd,
                config.warmup) != 0) {

            fprintf(stderr,
                    "[ERROR] Warmup failed.\n");

            close_packet_filter(fd);

            return EXIT_FAILURE;
        }
    }

    /*
     * --------------------------------------------------------
     * IOCTL latency
     * --------------------------------------------------------
     */

    if (config.ioctl_test) {

        if (benchmark_ioctl_latency(
                fd,
                &config,
                &ioctl_result) != 0) {

            fprintf(stderr,
                    "[ERROR] IOCTL benchmark failed.\n");

            close_packet_filter(fd);

            return EXIT_FAILURE;
        }

        ioctl_result_ptr =
            &ioctl_result;

        print_result(
            "IOCTL GET_STATUS Performance",
            &ioctl_result);
    }

    /*
     * --------------------------------------------------------
     * Statistics latency
     * --------------------------------------------------------
     */

    if (config.statistics_test) {

        if (benchmark_statistics(
                fd,
                &config,
                &stats_result) != 0) {

            fprintf(stderr,
                    "[ERROR] Statistics benchmark failed.\n");

            close_packet_filter(fd);

            return EXIT_FAILURE;
        }

        stats_result_ptr =
            &stats_result;

        print_result(
            "IOCTL GET_STATS Performance",
            &stats_result);

        print_packet_statistics(fd);
    }

    /*
     * --------------------------------------------------------
     * Packet buffer benchmark
     * --------------------------------------------------------
     */

    if (config.throughput_test) {

        if (benchmark_packet_buffer(
                &config,
                &throughput_result) != 0) {

            fprintf(stderr,
                    "[ERROR] Throughput benchmark failed.\n");

            if (fd >= 0) {
                close_packet_filter(fd);
            }

            return EXIT_FAILURE;
        }

        throughput_result_ptr =
            &throughput_result;

        print_result(
            "Packet Buffer Performance",
            &throughput_result);
    }

    /*
     * --------------------------------------------------------
     * Resource usage
     * --------------------------------------------------------
     */

    print_resource_usage();

    /*
     * --------------------------------------------------------
     * Final driver statistics
     * --------------------------------------------------------
     */

    if (fd >= 0) {

        printf("\n");
        printf("Final Packet Filter Statistics\n");
        printf("--------------------------------\n");

        print_packet_statistics(fd);

        /*
         * Disable driver if supported.
         */

        disable_driver(fd);

        close_packet_filter(fd);
    }

    /*
     * --------------------------------------------------------
     * Summary
     * --------------------------------------------------------
     */

    print_summary(
        ioctl_result_ptr,
        stats_result_ptr,
        throughput_result_ptr);

    printf("\n");
    printf("[PASS] Performance benchmark completed.\n");
    printf("\n");

    return EXIT_SUCCESS;
}
