#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION "0.1.4"

static char *verify_format(char *format, const char *descriptor,
	const char *prog_name);
static void print_help(const char *prog_name, FILE *to);
static void print_version(const char *prog_name, FILE *to);
/* Animate a duck with the given frames saying the given message, waiting a
 * given delay between each frame. */
static void print_animated(const char *open_mouth, const char *closed_mouth,
	const char *message, int delay);
/* Instantly print the duck saying the complete message, open-mouthed. */
static void print_unanimated(const char *open_mouth, const char *message);
int main(int argc, char *argv[])
{
	char *prog_name = argv[0];
	const char *open_mouth = "(^)= -{%.*s}";
	const char *closed_mouth = "(^)- -{%.*s}";
	char *message;
	enum {
		/* Decide whether to animate based on isatty. */
		ANIMATION_AUTO,
		/* use the print_unanimated function. */
		ANIMATION_NONE,
		/* use the print_animated function. */
		ANIMATION_FORCE
	} animation = ANIMATION_AUTO;
	int delay = 100000;
	int opt;
	while ((opt = getopt(argc, argv, "o:c:m:ANFhv")) != -1) {
		switch (opt) {
		case 'o':
			open_mouth = verify_format(optarg, "open-mouth",
				prog_name);
			break;
		case 'c':
			closed_mouth = verify_format(optarg, "closed-mouth",
				prog_name);
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

/* Escape all occurrences of '%' in a string so that it can be as a format. */
static char *escape(char *format)
{
	char *last_found = format;
	if ((format = strchr(format, '%'))) {
		size_t unescaped_length = strlen(last_found);
		/* Maximum length is 2x the original (if every char is '%'): */
		char *escaped = malloc(unescaped_length * 2 + 1);
		/* The index of escaped being written to: */
		size_t writing = 0;
		do {
			/* The length of the preserved segment: */
			size_t segment = (size_t)(format - last_found);
			++format;
			memcpy(escaped + writing, last_found, segment);
			writing += segment;
			memcpy(escaped + writing, "%%", 2);
			writing += 2;
			last_found = format;
		} while ((format = strchr(format, '%')));
		/* Copy final '%'-free segment: */
		strcpy(escaped + writing, last_found);
		return escaped;
	} else {
		return last_found;
	}
}
static char *verify_format(char *format, const char *descriptor,
	const char *prog_name)
{
	static const char original[] = "TEXT";
	static const char translation[] = "%.*s";
	format = escape(format);
	char *parameter = strstr(format, original);
	if (!parameter) {
		fprintf(stderr, "%s: the %s format must have one "
				"string parameter (TEXT).\n",
			prog_name, descriptor);
		print_help(prog_name, stderr);
		exit(EXIT_FAILURE);
	}
	memcpy(parameter, translation, 4);
	return format;
}

static void print_help(const char *prog_name, FILE *to)
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
		"Formats refer to strings with exactly one part being 'TEXT'. "
		"This part will\nbecome the message displayed. Example usage:\n"
		"duckt -o ':-O ~(TEXT)' -c ':-| ~(TEXT)'.\n",
		prog_name);
}

static void print_version(const char *prog_name, FILE *to)
{
	fprintf(to, "%s " VERSION "\n", prog_name);
}

/* Move forward one UTF-8 character, returning the delta. */
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
/* Print a frame and wait. */
static void print_step(const char *format, const char *message, int delay,
	size_t extent)
{
	printf("\e[u");
	printf(format, extent, message);
	fflush(stdout);
	printf("\r");
	usleep(delay);
}
static void print_animated(const char *open_mouth, const char *closed_mouth,
	const char *message, int delay)
{
	size_t length = strlen(message), extent = 0;
	printf("\e[s");
	while ((extent += advance(message, length, extent)) <= length) {
		print_step(open_mouth, message, delay, extent);
		print_step(closed_mouth, message, delay, extent);
	}
	printf("\n");
}

static void print_unanimated(const char *open_mouth, const char *message)
{
	printf(open_mouth, strlen(message), message);
	printf("\n");
}
