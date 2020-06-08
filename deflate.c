#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level, int mem_level, bool ended) {
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, level, Z_DEFLATED, -15, mem_level, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void) deflateEnd(&strm);
            return Z_ERRNO;
        }
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            int flush = ended ? Z_FINISH : Z_SYNC_FLUSH;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void) deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */

        if (feof(source)) {
            break;
        }
    } while (1);

    /* clean up and return */
    (void) deflateEnd(&strm);
    return Z_OK;
}


/* report a zlib or i/o error */
void zerr(int ret) {
    fputs("deflate: ", stderr);
    switch (ret) {
        case Z_ERRNO:
            if (ferror(stdin))
                fputs("error reading stdin\n", stderr);
            if (ferror(stdout))
                fputs("error writing stdout\n", stderr);
            break;
        case Z_STREAM_ERROR:
            fputs("invalid compression level\n", stderr);
            break;
        case Z_DATA_ERROR:
            fputs("invalid or incomplete deflate data\n", stderr);
            break;
        case Z_MEM_ERROR:
            fputs("out of memory\n", stderr);
            break;
        case Z_VERSION_ERROR:
            fputs("zlib version mismatch!\n", stderr);
    }
}

/*
 * -0 to -9        Compression level
 * --fast, --best       Compression levels 1 and 9 respectively
 * -b, --blocksize mmm  Set compression block size to mmmK (default 128K)
-c, --stdout         Write all processed output to stdout (won't delete)
  -c     : force write to standard output, even if it is the console
-d, --decompress     Decompress the compressed input (default for .lz4 extension)
 x
 -k, --keep           Do not delete original file after processing
  -m     : multiple input files (implies automatic output filenames)
-r     : operate recursively on directories (sets also -m)
 -S, --suffix .sss    Use suffix .sss instead of .gz (for compression)
 -V  --version        Show the version of pigz
 --                   All arguments after "--" are treated as files

 -o FILE, --output=FILE      output file (only if 1 input file)
With no FILE, or when FILE is -, read standard input.

 -z --compress       force compression
 -s --small          use less memory (at most 2500k)
 -E --empty
 -F --finish (busy -F  --first          Do iterations first, before block split for -11)

 -C checksum or chunk
 -X (extensiblae)

 yu

 -A appendable
 -L last

 checksum(s) for each stream
 output to gz (G is busy)



 deflate header.txt.deflate -0 entry1.txt entry.txt footer.deflate
	 error: should we compress?
 deflate -z header.txt.deflate -0 entry1.txt entry.txt footer.deflate
	 last stream is opened
 deflate -z -F header.txt.deflate -0 entry1.txt entry.txt footer.deflate
	 last stream is finished
 deflate -z -F header.txt.deflate -0 entry1.txt entry.txt -0  footer.deflate
 */

#define SHORT_OPTS "zdkcS:o:f0123456789sAEqvhV"
#define DEFLATE_VERSION "0.1.0"

#ifndef NO_HELP
enum {
    L_COMPRESS,
    L_DECOMPRESS,
    L_KEEP,
    L_STDOUT,
    L_SUFFIX,
    L_OUTPUT,
    L_FORCE,
    L_FAST,
    L_BEST,
    L_SMALL,
    L_APPENDABLE,
    L_ENDED,
    L_QUIET,
    L_VERBOSE,
    L_HELP,
    L_VERSION,
};
#define OPTS_COUNT L_VERSION - L_COMPRESS

static const struct option longopts[] = {
        [L_COMPRESS] = {"compress", no_argument, NULL, 'z'},
        [L_DECOMPRESS] = {"decompress", no_argument, NULL, 'd'},
        [L_KEEP] = {"keep", no_argument, NULL, 'k'},
        [L_STDOUT] = {"stdout", no_argument, NULL, 'c'},
        [L_SUFFIX] = {"suffix", required_argument, NULL, 'S'},
        [L_OUTPUT] = {"output", required_argument, NULL, 'o'},
        [L_FORCE] = {"force", no_argument, NULL, 'f'},
        [L_FAST] = {"fast", no_argument, NULL, '1'},
        [L_BEST] = {"best", no_argument, NULL, '9'},
        [L_SMALL] = {"small", no_argument, NULL, 's'},
        [L_APPENDABLE] = {"appendable", no_argument, NULL, 'A'},
        [L_ENDED] = {"ended", no_argument, NULL, 'E'},
        [L_QUIET] = {"quiet", no_argument, NULL, 'q'},
        [L_VERBOSE] = {"verbose", no_argument, NULL, 'v'},
        [L_HELP] = {"help", no_argument, NULL, 'h'},
        [L_VERSION] = {"version", no_argument, NULL, 'V'},
        {}
};
#endif

#ifdef HELP_DESCRIPTION
struct option_desc {
    const char *def_val;
    const char *desc;
};

static const struct option_desc opts_desc[] = {
        [L_COMPRESS] = {"yes, if the input file extension is not .deflate", "Force compression"},
        [L_DECOMPRESS] = {"yes, if the input file extension is .deflate", "Force decompression"},
        [L_KEEP] = {NULL, "Don't delete input files"},
        [L_STDOUT] = {NULL, "Force write to standard output, even if it is the console"},
        [L_SUFFIX] = {".deflate", "Use suffix .sss instead of .deflate (for compression)"},
        [L_OUTPUT] = {NULL, "Output file path"},
        [L_FORCE] = {NULL, "Force output file overwrite"},
        [L_FAST] = {NULL, "Compress faster i.e. the same as -1"},
        [L_BEST] = {"yes", "Use best compression level i.e. the same as -9"},
        [L_SMALL] = {NULL, "Use less memory i.e. memLevel = 1. By default memLevel = 8"},
        [L_APPENDABLE] = {"yes", "Add padding with zero blocks so it can be concatenated"},
        [L_ENDED] = {NULL, "Ended and non appendable stream without zero blocks in the end so it's smaller but can't be concatenated"},
        [L_QUIET] = {NULL, "Print no messages, even on error"},
        [L_VERBOSE] = {NULL, "Provide more verbose output"},
        [L_HELP] = {NULL, "Display this help"},
        [L_VERSION] = {NULL, "Show the program version"},
};
#endif

void print_version() {
    fprintf(stderr, "deflate %s zlib %s\n", DEFLATE_VERSION, zlibVersion());
}

void print_usage() {
    print_version();
#ifndef NO_HELP
    fputs("usage: \n"
          "deflate f1.txt [f2.txt [f3.txt ...]] -o full.txt.gz\n"
          "deflate -d full.txt.gz -o full.txt\n"
          "All options: " SHORT_OPTS "\n",
          stderr);

    int i;
    for (i = 0; i < OPTS_COUNT; i++) {
        struct option cur_opt;
        cur_opt = longopts[i];
        fprintf(stderr, "  -%c --%s", cur_opt.val, cur_opt.name);
#ifdef HELP_DESCRIPTION
        struct option_desc cur_option_desc;
        cur_option_desc = opts_desc[i];
        fprintf(stderr, " %s", cur_option_desc.desc);
        if (cur_option_desc.def_val) {
            fprintf(stderr, ". Default: %s", cur_option_desc.def_val);
        }
#endif
        fprintf(stderr, "\n");
    }
#endif
}

struct GlobalArgs {
    int verbosity; /* -v and -q option */
    const char *out_file_name; /* -o option */
    bool force;
    char **input_files; /* input files */
    int input_files_count; /* # of input files */
    bool decompress;
    bool keep;
    char *suffix;
    int compression_level; /* --fast and --best */
    int mem_level; /* --small */
    bool ended;
};

void print_opts(struct GlobalArgs *global_args) {
    char **input_files;

    fprintf(stderr, "output file: %s\n"
                    "compression level: %d\n"
                    "mem_level: %d\n"
                    "ended: %d\n",
            global_args->out_file_name, global_args->compression_level, global_args->mem_level, global_args->ended);

    input_files = global_args->input_files;
    for (int i = 0; i < global_args->input_files_count; i++) {
        fprintf(stderr, "%s\n", input_files[i]);
    }
}

char *concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator
    return result;
}

/* compress or decompress from stdin to stdout */
int main(int argc, char **argv) {
    int ret;
    int opt;

    /* Initialize global_args before we get to work. */
    struct GlobalArgs global_args = {
            1,     //verbosity
            NULL,     //out_file_name
            false,     //input_files
            NULL,     //input_files
            0,     //input_files_count
            false,     //decompress
            false,     //keep
            NULL,     //*suffix ".deflate"
            Z_BEST_COMPRESSION,     //compression_level
            8,     //mem_level
            false     //ended
    };

#ifdef NO_HELP
        while ((opt = getopt(argc, argv, SHORT_OPTS)) != -1) {
#else
        int longopt_idx = 0;
        while ((opt = getopt_long(argc, argv, SHORT_OPTS, longopts, &longopt_idx)) != -1) {
#endif
        switch (opt) {
            case 'h':
                print_usage();
                return 0;
            case 'V':
                print_version();
                return 0;
            case 'q':
                global_args.verbosity = 0;
                break;
            case 'v':
                global_args.verbosity = 2;
                break;
            case 'z':
                global_args.decompress = false;
                break;
            case 'd':
                global_args.decompress = true;
                break;
            case 'k':
                global_args.keep = true;
                break;
            case 'c':
                global_args.out_file_name = "-";
                break;
            case 'S':
                global_args.suffix = optarg;
                break;
            case 'o':
                global_args.out_file_name = optarg;
                break;
            case 'f':
                global_args.force = true;
                break;
            case 's':
                global_args.mem_level = 1;
                break;
            case 'A':
                global_args.ended = false;
                break;
            case 'E':
                global_args.ended = true;
                break;
            case '0':
                global_args.compression_level = 0;
                break;
            case '1':
                global_args.compression_level = 1;
                break;
            case '2':
                global_args.compression_level = 2;
                break;
            case '3':
                global_args.compression_level = 3;
                break;
            case '4':
                global_args.compression_level = 4;
                break;
            case '5':
                global_args.compression_level = 5;
                break;
            case '6':
                global_args.compression_level = 6;
                break;
            case '7':
                global_args.compression_level = 7;
                break;
            case '8':
                global_args.compression_level = 8;
                break;
            case '9':
                global_args.compression_level = 9;
                break;
            default:
                print_usage();
                return 0;
        }
    }

    global_args.input_files = argv + optind;
    global_args.input_files_count = argc - optind;

    if (global_args.verbosity >= 1) {
        print_version();
    }
    if (global_args.verbosity >= 2) {
        print_opts(&global_args);
    }

    char *input_file_name;

    input_file_name = global_args.input_files != NULL ? *global_args.input_files : NULL;

    FILE *out_file;
    int is_piped = !isatty(STDOUT_FILENO);
    bool out_file_name_is_stdout = global_args.out_file_name ? !strcmp("-", global_args.out_file_name) : false;
    bool no_input_file_name_to_derive_output = !input_file_name && !global_args.out_file_name;
    if (global_args.verbosity >= 2) {
        fprintf(stderr, "is_piped %d\n"
                        "out_file_name_is_stdout %d\n"
                        "no_input_file_name_to_derive_output %d\n",
                is_piped, out_file_name_is_stdout, no_input_file_name_to_derive_output);
    }
    if (is_piped || out_file_name_is_stdout || no_input_file_name_to_derive_output) {
        out_file = stdout;
    } else {
        const char *out_file_name;
        char *suffix = global_args.suffix;
        if (global_args.out_file_name) {
            out_file_name = global_args.out_file_name;
        } else {
            out_file_name = input_file_name;
            suffix = suffix ? suffix : ".deflate";
        }
        if (suffix != NULL) {
            out_file_name = concat(out_file_name, suffix);
        }
        if (!global_args.force && access(out_file_name, F_OK) != -1) {
            if (global_args.verbosity >= 1) {
                fprintf(stderr, "File already exists. Use -f (--force) to overwrite it: %s\n", out_file_name);
            }
            return 1;
        }
        out_file = fopen(out_file_name, "w+");
    }

    FILE *in_file = input_file_name ? fopen(input_file_name, "r") : stdin;
    ret = def(in_file, out_file, global_args.compression_level, global_args.mem_level, global_args.ended);
    fclose(in_file);
    fclose(out_file);
    if (ret != Z_OK)
        zerr(ret);
    return EXIT_SUCCESS;
}
