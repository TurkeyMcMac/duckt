#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION "0.0.1"

void verify_format(char *format, const char *descriptor,
	const char *prog_name);
void print_help(const char *prog_name, FILE *to);
void print_version(const char *prog_name, FILE *to);
void print_animated(const char *open_mouth, const char *closed_mouth,
	const char *message, int delay);
void print_unanimated(const char *open_mouth, const char *message);
int main(int argc, char *argv[])
{
	char *prog_name = argv[0];
	const char *open_mouth = "(^)= -{%.*s}";
	const char *closed_mouth = "(^)- -{%.*s}";
	char *message;
	enum {
		ANIMATION_AUTO,
		ANIMATION_NONE,
		ANIMATION_FORCE
	} animation = ANIMATION_AUTO;
	int delay = 100000;
	int opt;
	while ((opt = getopt(argc, argv, "o:c:m:ANFhv")) != -1) {
		switch (opt) {
		case 'o':
			verify_format(optarg, "open-mouth", prog_name);
			open_mouth = optarg;
			break;
		case 'c':
			verify_format(optarg, "closed-mouth", prog_name);
			closed_mouth = optarg;
			break;
		case 'm':
			delay = atoi(optarg);
			break;
		case 'A':
			animation = ANIMATION_AUTO;
			break;
		case 'N':
			animation = ANIMATION_NONE;
			break;
		case 'F':
			animation = ANIMATION_FORCE;
			break;
		case 'h':
			print_help(prog_name, stdout);
			return EXIT_SUCCESS;
		case 'v':
			print_version(prog_name, stdout);
			return EXIT_SUCCESS;
		default:
			print_help(prog_name, stderr);
			return EXIT_FAILURE;
		}
	}
	message = argv[optind];
	if (!message) {
		message = "";
	}
	if (animation == ANIMATION_AUTO) {
		if (isatty(STDOUT_FILENO)) {
			animation = ANIMATION_FORCE;
		}
		else {
			animation = ANIMATION_NONE;
		}
	}
	if (animation == ANIMATION_FORCE) {
		print_animated(open_mouth, closed_mouth, message, delay);
	} else {
		print_unanimated(open_mouth, message);
	}
}

void verify_format(char *format, const char *descriptor,
	const char *prog_name)
{
	static const char original[] = "TEXT";
	static const char translation[] = "%.*s";
	char *parameter = strstr(format, original);
	if (!parameter || strstr(parameter + 4, original)) {
		fprintf(stderr, "%s: the %s format must have exactly one "
				"string parameter (TEXT).\n",
			prog_name, descriptor);
		print_help(prog_name, stderr);
		exit(EXIT_FAILURE);
	}
	memcpy(parameter, translation, 4);
}

void print_help(const char *prog_name, FILE *to)
{
	fprintf(to,
		"Usage: %s [options] [message]\n"
		"Options:\n"
		"  -o <open>   Specify the message format when the mouth is "
		              "open.\n"
		"  -c <closed> Specify the message format with mouth closed.\n"
		"  -m <delay>  Specify the delay in milliseconds.\n"
		"  -A          Automatically decide whether or not to animate. "
		              "This option is on\n"
		"              by default.\n"
		"  -N          Never animate output.\n"
		"  -F          Force output animation. This does weird stuff "
		              "if the output is\n"
		"              piped.\n"
		"  -h          Display this help information.\n"
		"  -v          Display version information.\n"
		"Formats refer to strings with exactly one part being '%%s'. "
		"This part will become\n"
		"the message displayed.\n",
		prog_name);
}

void print_version(const char *prog_name, FILE *to)
{
	fprintf(to, "%s " VERSION "\n", prog_name);
}

static size_t advance(const char *message, size_t length, size_t extent)
{
	if (message[extent] >= 0) {
		return 1;
	}
	unsigned char_bits = (unsigned)message[extent]
		<< ((sizeof(unsigned) - 1) * CHAR_BIT);
	size_t char_size = __builtin_clz(~char_bits);
	return char_size;
}
static void print_step(const char *format, const char *message, int delay,
	size_t extent)
{
	printf(format, extent, message);
	fflush(stdout);
	usleep(delay);
	printf("\r");
}
void print_animated(const char *open_mouth, const char *closed_mouth,
	const char *message, int delay)
{
	size_t length = strlen(message), extent = 0;
	while ((extent += advance(message, length, extent)) <= length) {
		print_step(open_mouth, message, delay, extent);
		print_step(closed_mouth, message, delay, extent);
	}
	printf("\n");
}

void print_unanimated(const char *open_mouth, const char *message)
{
	printf(open_mouth, strlen(message), message);
	printf("\n");
}
