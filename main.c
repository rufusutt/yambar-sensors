#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <sensors/sensors.h>
#include <sensors/error.h>

struct subfeature_list {
    struct subfeature_list *next;
    const sensors_subfeature *subfeature;
};

static int setup_signal_handler(void);
static void sigint_handler(int signum);
static int prepend_subfeature(struct subfeature_list **list,
                              const sensors_subfeature *subfeature);
static void reverse_subfeature_list(struct subfeature_list **list);
static void free_subfeature_list(struct subfeature_list *list);
static int initialise_sensors(void);
static int parse_chip_name(const char *name, sensors_chip_name *match);
static int parse_interval(const char *str, int *interval);
static const sensors_chip_name *get_matching_chip(const sensors_chip_name *match);
static void cleanup_resources(struct subfeature_list *subfeatures,
                              sensors_chip_name *match);
static int collect_subfeatures(const sensors_chip_name *name,
                               const sensors_feature *feature,
                               struct subfeature_list **list);
static int collect_all_subfeatures(const sensors_chip_name *name,
                                   struct subfeature_list **list);
static void print_sensor_values(const sensors_chip_name *chip,
                                struct subfeature_list *subfeatures);

static volatile sig_atomic_t sigint_flag = 0;
static const struct sigaction sigint_sa = {
    .sa_handler = sigint_handler,
};

int main(int argc, char **argv) {
    struct subfeature_list *subfeatures = NULL;
    sensors_chip_name match;
    const sensors_chip_name *chip;
    int ret, interval;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <chip> <interval>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_interval(argv[2], &interval) != 0) {
        return EXIT_FAILURE;
    }

    if (initialise_sensors() != 0) {
        return EXIT_FAILURE;
    }

    if (parse_chip_name(argv[1], &match) != 0) {
        sensors_cleanup();
        return EXIT_FAILURE;
    }

    chip = get_matching_chip(&match);
    if (!chip) {
        sensors_free_chip_name(&match);
        sensors_cleanup();
        return EXIT_FAILURE;
    }

    ret = collect_all_subfeatures(chip, &subfeatures);
    if (ret) {
        fprintf(stderr, "Error collecting subfeatures\n");
        sensors_free_chip_name(&match);
        sensors_cleanup();
        return EXIT_FAILURE;
    }
    reverse_subfeature_list(&subfeatures);

    if (setup_signal_handler() != 0) {
        cleanup_resources(subfeatures, &match);
        return EXIT_FAILURE;
    }

    while (!sigint_flag) {
        print_sensor_values(chip, subfeatures);
        sleep(interval);
    }

    fprintf(stderr, "Exiting...\n");
    cleanup_resources(subfeatures, &match);
    return EXIT_SUCCESS;
}

static int setup_signal_handler(void)
{
    if (sigaction(SIGINT, &sigint_sa, NULL)) {
        perror("Error setting up signal handler");
        return -1;
    }
    return 0;
}

static void sigint_handler(int signum)
{
    (void)signum;
    sigint_flag = 1;
}

static int prepend_subfeature(struct subfeature_list **list,
                              const sensors_subfeature *subfeature)
{
    struct subfeature_list *new = malloc(sizeof(struct subfeature_list));
    if (!new) {
        return -1;
    }
    new->subfeature = subfeature;
    new->next = *list;
    *list = new;
    return 0;
}

static void reverse_subfeature_list(struct subfeature_list **list)
{
    struct subfeature_list *prev = NULL;
    struct subfeature_list *current = *list;
    struct subfeature_list *next;
    while (current) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    *list = prev;
}

static void free_subfeature_list(struct subfeature_list *list)
{
    struct subfeature_list *next;
    while (list) {
        next = list->next;
        free(list);
        list = next;
    }
}

static int initialise_sensors(void)
{
    int ret = sensors_init(NULL);
    if (ret) {
        fprintf(stderr, "Error initialising sensors: %s\n",
                sensors_strerror(ret));
        return -1;
    }
    return 0;
}

static int parse_chip_name(const char *name, sensors_chip_name *match)
{
    int ret = sensors_parse_chip_name(name, match);
    if (ret) {
        fprintf(stderr, "Error parsing chip name: %s\n",
                sensors_strerror(ret));
        return -1;
    }
    return 0;
}

static int parse_interval(const char *str, int *interval)
{
    char *endptr;
    long val;

    errno = 0;
    val = strtol(str, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
        perror("Error parsing interval");
        return -1;
    }

    if (endptr == str) {
        fprintf(stderr, "No digits found in interval\n");
        return -1;
    }

    if (*endptr != '\0') {
        fprintf(stderr, "Invalid characters found in interval\n");
        return -1;
    }

    if (val <= 0 || val > INT_MAX) {
        fprintf(stderr, "Interval must be a positive integer\n");
        return -1;
    }

    *interval = val;
    return 0;
}

static const sensors_chip_name *get_matching_chip(const sensors_chip_name *match)
{
    int nr = 0;
    const sensors_chip_name *chip = sensors_get_detected_chips(match, &nr);
    if (!chip) {
        fprintf(stderr, "No matching chip found\n");
        return NULL;
    }
    return chip;
}

static void cleanup_resources(struct subfeature_list *subfeatures,
                              sensors_chip_name *match)
{
    free_subfeature_list(subfeatures);
    sensors_free_chip_name(match);
    sensors_cleanup();
}

static int collect_subfeatures(const sensors_chip_name *name,
                               const sensors_feature *feature,
                               struct subfeature_list **list)
{
    const sensors_subfeature *subfeature;
    int nr = 0;
    do {
        subfeature = sensors_get_all_subfeatures(name, feature, &nr);
        if (subfeature && subfeature->flags & SENSORS_MODE_R) {
            if (prepend_subfeature(list, subfeature)) {
                free_subfeature_list(*list);
                return -1;
            }
        }
    } while (subfeature);
    return 0;
}

static int collect_all_subfeatures(const sensors_chip_name *name,
                                   struct subfeature_list **list)
{
    const sensors_feature *feature;
    int nr = 0;
    do {
        feature = sensors_get_features(name, &nr);
        if (feature) {
            if (collect_subfeatures(name, feature, list)) {
                free_subfeature_list(*list);
                return -1;
            }
        }
    } while (feature);
    return 0;
}

static void print_sensor_values(const sensors_chip_name *chip,
                                struct subfeature_list *subfeatures)
{
    struct subfeature_list *current = subfeatures;
    while (current) {
        const sensors_subfeature *subfeature = current->subfeature;

        double value;
        int ret = sensors_get_value(chip, subfeature->number, &value);
        if (ret) {
            fprintf(stderr, "Error getting value for %s: %s\n",
                    subfeature->name, sensors_strerror(ret));
        } else {
            printf("%s|float|%f\n", subfeature->name, value);
        }

        current = current->next;
    }
    printf("\n");
    fflush(stdout);
}
