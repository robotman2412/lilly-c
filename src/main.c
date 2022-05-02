
#include "main.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <asm.h>
#include <config.h>
#include "tokeniser.h"
#include "parser.h"
#include <gen.h>
#include "asm_postproc.h"

#include "stdlib.h"
#include "errno.h"

#ifdef DEBUG_COMPILER
#include "pront.h"
#include "gen_tests.h"
#endif

// Filling in of an external array.
char *reg_names[] = REG_NAMES;

// Filling in of another external array.
size_t simple_type_size[] = arrCSIZE_BY_INDEX;

typedef struct token {
	enum yytokentype type;
	YYSTYPE val;
} token_t;

int main(int argc, char **argv) {
#if defined(ENABLE_DEBUG_LOGS) && YYDEBUG
	yydebug = 1;
#endif
	
	options_t options = {
		.abort          = false,
		.showHelp       = false,
		.showVersion    = false,
		.numSourceFiles = 0,
		.sourceFiles    = NULL,
		.numIncludeDirs = 0,
		.includeDirs    = NULL,
		.outputFile     = NULL,
		.outputType     = NULL
	};
	
	// Read options.
	int argIndex;
	for (argIndex = 1; argIndex < argc; argIndex++) {
		if (!strcmp(argv[argIndex], "-v") || !strcmp(argv[argIndex], "--version")) {
			// Show version
			if (options.showVersion) printf("Note: Show version already specified.\n");
			options.showVersion = true;
			
		} else if (!strcmp(argv[argIndex], "-h") || !strcmp(argv[argIndex], "--help")) {
			// Show help.
			if (options.showHelp) printf("Note: Show help already specified.\n");
			options.showHelp = true;
			
		} else if (!strncmp(argv[argIndex], "-I", 2)) {
			// Add include directory.
			char *dir = &(argv[argIndex])[2];
			if (isdir(dir)) {
				options.numIncludeDirs ++;
				options.includeDirs = (char **) realloc(options.includeDirs, sizeof(char *) * options.numIncludeDirs);
				options.includeDirs[options.numIncludeDirs - 1] = dir;
			} else if (access(dir, R_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: '%s' is not a directory\n", dir);
				options.abort = true;
			} else if (access(dir, F_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: Permission denied for '%s'\n", dir);
				options.abort = true;
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: No such file or directory '%s'\n", dir);
				options.abort = true;
			}
		} else if (!strcmp(argv[argIndex], "-o")) {
			// Specify output file.
			if (argIndex < argc - 1) {
				argIndex ++;
				printf("%s\n", argv[argIndex]);
				if (access(argv[argIndex], W_OK)) {
					options.outputFile = argv[argIndex];
				} else if (isdir(argv[argIndex])) {
					fflush(stdout);
					fprintf(stderr, "Error: '%s' is a directory\n", argv[argIndex]);
					options.abort = true;
				} else if (access(argv[argIndex], F_OK)) {
					fflush(stdout);
					fprintf(stderr, "Error: Permission denied for '%s'\n", argv[argIndex]);
					options.abort = true;
				}
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: Missing filename for '-o'\n");
				options.abort = true;
			}
		} else if (!strncmp(argv[argIndex], "--include=", 10)) {
			// Add include directory.
			options.numIncludeDirs ++;
			options.includeDirs = (char **) realloc(options.includeDirs, sizeof(char *) * options.numIncludeDirs);
			options.includeDirs[options.numIncludeDirs - 1] = &(argv[argIndex])[10];
		} else if (!strncmp(argv[argIndex], "--outtype=", 10)) {
			// Output type.
			if (options.outputType) printf("Note: Output type already specified.\n");
			options.outputType = &(argv[argIndex])[6];
		} else if (*argv[argIndex] == '-') {
			// Unknown option.
			fflush(stdout);
			fprintf(stderr, "Error: Unknown option '%s'!\n", argv[argIndex]);
			options.showHelp = true;
		} else {
			// End of options.
			break;
		}
	}
	apply_defaults(&options);
	
#if defined(FUNC_TEST) || defined(EXPR_TEST)
	perform_gen_tests(argc, argv);
	return 0;
#endif
	
	if (options.showHelp) {
		printf("lilly-cc " ARCH_ID " " COMPILER_VER "\n");
		show_help(argc, argv);
		return 0;
	}
	if (options.showVersion) {
		printf("lilly-cc " ARCH_ID " " COMPILER_VER "\n");
		if (argIndex >= argc) return 0;
	}
	if (options.abort) {
		return 1;
	}
	
	// Check for input files.
	if (argIndex >= argc) {
		fflush(stdout);
		fprintf(stderr, "Error: No input files specified!\n");
		fprintf(stderr, "Try '%s --help' for help.\n", *argv);
		return 0;
	}
	
	// Read file.
	if (argc > argIndex + 1) printf("Please note: Only the first input file is compiled for now.\n");
	char *filename = argv[argIndex];
	FILE *file = fopen(filename, "r");
	if (!file) {
		fflush(stdout);
		fprintf(stderr, "Error: Cannot access %s: %s\n", argv[argIndex], strerror(errno));
		return 0;
	}
	
	// Have it PROCESSED.
	asm_ctx_t *asm_ctx = compile(filename, file);
	if (!asm_ctx) {
		return 1;
	}
	
	// Output it.
	FILE *tempfile = fopen("/tmp/lilly-cc-dbg-bin", "w+");
	// chmod("/tmp/lilly-cc-dbg-bin", 0666);
	asm_ctx->out_fd = tempfile;
	asm_ppc(asm_ctx);
	output_native(asm_ctx);
	fclose(tempfile);
	free(asm_ctx);
	system("hexdump -ve '8/2 \"%04X \" \"\n\"' /tmp/lilly-cc-dbg-bin");
}

// Show help on the command line.
void show_help(int argc, char **argv) {
	printf("%s [options] source-files...\n", *argv);
	printf("Options:\n");
	printf("  -v  --version\n");
	printf("                Show the version.\n");
	printf("  -h  --help\n");
	printf("                Show this list.\n");
	printf("  -o <file>\n");
	printf("                Specify the output file path.\n");
	printf("  -I<dir>  --include=<dir>\n");
	printf("                Add a directory to the include directories.\n");
	printf("  --outtype=<type>\n");
	printf("                Specify the output type:\n");
	printf("                shared, raw, executable\n");
}

// Apply default options for options not already set.
void apply_defaults(options_t *options) {
	if (!options->outputFile) {
		options->outputFile = "a.out";
		options->outputType = "executable";
	} else if (!options->outputType) {
		size_t len = strlen(options->outputFile);
		if (!strcmp(&options->outputFile[len - 3], ".so")) {
			options->outputType = "shared";
		} else if (!strcmp(&options->outputFile[len - 3], ".o")) {
			options->outputType = "shared";
		} else if (!strcmp(&options->outputFile[len - 4], ".bin")) {
			options->outputType = "raw";
		} else {
			options->outputType = "executable";
		}
	}
}

// Check wether a file exists and is a directory.
bool isdir(char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) != 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

// Compile a file of unknown type.
asm_ctx_t *compile(char *filename, FILE *file) {
	char *dot = strrchr(filename, '.');
	if (!dot) {
		printf("%s: Filetype not recognised.\n", filename);
		return NULL;
	} else if (!strcmp(dot, ".c")) {
		return compile_c(filename, file);
	} else if (!strcmp(dot, ".s") || !strcmp(dot, ".asm")) {
		return assemble_s(filename, file);
	} else {
		printf("%s: Filetype not recognised.\n", filename);
		return NULL;
	}
}

// Compile a C source file.
asm_ctx_t *compile_c(char *filename, FILE *file) {
	// Init some ctx.
	parser_ctx_t    ctx;
	tokeniser_ctx_t tokeniser_ctx;
	asm_ctx_t       asm_ctx;
	ctx.tokeniser_ctx = &tokeniser_ctx;
	ctx.asm_ctx       = &asm_ctx;
	tokeniser_init_file(&tokeniser_ctx, file);
	tokeniser_ctx.filename = filename;
	asm_init(&asm_ctx);
	
	// Compile some CRAP.
	yyparse(&ctx);
	
	return COPY(&asm_ctx, asm_ctx_t);
}

// Assembles an assembly source file.
asm_ctx_t *assemble_s(char *filename, FILE *file) {
	// Init some ctx.
	tokeniser_ctx_t tokeniser_ctx;
	asm_ctx_t       asm_ctx;
	tokeniser_init_file(&tokeniser_ctx, file);
	tokeniser_ctx.filename = filename;
	asm_init(&asm_ctx);
	
	// Compile some CRAP.
	gen_asm_file(&asm_ctx, &tokeniser_ctx);
	
	return COPY(&asm_ctx, asm_ctx_t);
}

int yylex(parser_ctx_t *ctx) {
	int tkn = tokenise(ctx->tokeniser_ctx);
	return tkn;
}

void yyerror(parser_ctx_t *ctx, char *msg) {
	report_error(ctx->tokeniser_ctx, "error", yylval.pos, msg);
}

// Process a function.
void function_added(parser_ctx_t *parser_ctx, funcdef_t *func) {
	gen_function(parser_ctx->asm_ctx, func);
}

