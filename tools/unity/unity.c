/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 *
 * Source: https://github.com/adel-mamin/amast
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/common/macros.h"
#include "libs/strlib/strlib.h"

#define DB_FILES_MAX 256
#define MAX_INCLUDES_NUM 256
#define MAX_CONTENT_SIZE (128 * 1024)

struct files {
    char includes_std[MAX_INCLUDES_NUM][PATH_MAX];
    int includes_std_num;

    char fnames[DB_FILES_MAX][PATH_MAX];
    char content[DB_FILES_MAX][MAX_CONTENT_SIZE];
    int len;
};

struct db {
    struct files src;
    struct files src_test;
    struct files src_freertos;
    struct files src_posix;
    struct files src_cooperative;
    struct files src_preemptive;
    struct files hdr;
    struct files hdr_test;
    const char *odir; /* amast(-test).h and amast(-test).c are placed here */
};

static struct db m_db = {.src.len = 0, .hdr.len = 0};

static bool is_pragma(const char *str, const char *pragma) {
    if (NULL == strstr(str, "amast-pragma")) {
        return false;
    }
    return strstr(str, pragma) != NULL;
}

/* check if the include already exists in the array */
/* cppcheck-suppress-begin constParameter */
static bool include_is_unique(
    char arr[MAX_INCLUDES_NUM][PATH_MAX], int arr_size, const char *inc_file
) {
    for (int i = 0; i < arr_size; i++) {
        if (strcmp(arr[i], inc_file) == 0) {
            return false;
        }
    }
    return true;
}
/* cppcheck-suppress-end constParameter */

/* add unique include to the array */
static void include_add_unique(
    char arr[MAX_INCLUDES_NUM][PATH_MAX], int *arr_size, const char *inc_file
) {
    if (include_is_unique(arr, *arr_size, inc_file)) {
        str_lcpy(arr[*arr_size], inc_file, sizeof(arr[*arr_size]));
        (*arr_size)++;
    }
}

/* process a line and detect #include directives */
static void process_content(
    struct files *db, const char *ln, bool verbatim_include_std
) {
    char inc_file[PATH_MAX + 1];
#define AM_LIM AM_STRINGIFY(PATH_MAX)
    if (verbatim_include_std) {
        str_lcpy(db->includes_std[db->includes_std_num++], ln, PATH_MAX);
    } else if (sscanf(ln, "#include <%" AM_LIM "[^>]>%*s", inc_file) == 1) {
        include_add_unique(db->includes_std, &db->includes_std_num, inc_file);
    } else if (sscanf(ln, "#include \"%" AM_LIM "[^\"]\"%*s", inc_file) == 1) {
        /* ignore user includes */;
    } else { /* non-include line */
        str_lcat(db->content[db->len], ln, sizeof(db->content[db->len]));
    }
}

/* read the content of a file and process it */
static void read_file(struct files *db, const char *fname) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", fname);
        exit(EXIT_FAILURE);
    }

    char line[1024];
    bool verbatim_include_std = false;
    while (fgets(line, sizeof(line), file)) {
        if (verbatim_include_std) {
            if (is_pragma(line, "verbatim-include-std-off")) {
                verbatim_include_std = false;
                continue;
            }
        } else if (is_pragma(line, "verbatim-include-std-on")) {
            verbatim_include_std = true;
            continue;
        }
        process_content(db, line, verbatim_include_std);
    }

    fclose(file);

    strncpy(db->fnames[db->len], fname, PATH_MAX - 1);
}

/* comparator for qsort to sort includes alphabetically */
static int compare_includes(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

static void db_init(struct db *db, const char *db_fname, const char *odir) {
    FILE *file = fopen(db_fname, "r");
    if (!file) {
        fprintf(stderr, "Error: failed to open %s\n", db_fname);
        exit(EXIT_FAILURE);
    }

    char fname[PATH_MAX];
    while (fgets(fname, sizeof(fname), file)) {
        /* Remove trailing newline if present */
        fname[strcspn(fname, "\n")] = 0;

        if (strstr(fname, ".c") != NULL) {
            struct files *files = &db->src;
            if (strstr(fname, "test") != NULL) {
                files = &db->src_test;
            } else if (strstr(fname, "/libs/pal/freertos/") != NULL) {
                files = &db->src_freertos;
            } else if (strstr(fname, "/libs/pal/posix/") != NULL) {
                files = &db->src_posix;
            } else if (strstr(fname, "/libs/pal/stubs/") != NULL) {
                files = &db->src_test;
            } else if (strstr(fname, "/libs/ao/cooperative/") != NULL) {
                files = &db->src_cooperative;
            } else if (strstr(fname, "/libs/ao/preemptive/") != NULL) {
                files = &db->src_preemptive;
            }
            AM_ASSERT(files->len < AM_COUNTOF(files->content));
            read_file(files, fname);
            files->len++;
            continue;
        }
        if (strstr(fname, ".h") != NULL) {
            if (strstr(fname, "test") != NULL) {
                AM_ASSERT(db->hdr_test.len < AM_COUNTOF(db->hdr_test.content));
                read_file(&db->hdr_test, fname);
                db->hdr_test.len++;
            } else {
                AM_ASSERT(db->hdr.len < AM_COUNTOF(db->hdr.content));
                read_file(&db->hdr, fname);
                db->hdr.len++;
            }
            continue;
        }
        if ('\0' == fname[0]) {
            continue;
        }
        AM_ASSERT(0);
    }

    fclose(file);
    db->odir = odir;

    qsort(
        db->src.includes_std,
        (size_t)db->src.includes_std_num,
        sizeof(db->src.includes_std[0]),
        compare_includes
    );
    qsort(
        db->src_test.includes_std,
        (size_t)db->src_test.includes_std_num,
        sizeof(db->src_test.includes_std[0]),
        compare_includes
    );
    qsort(
        db->hdr.includes_std,
        (size_t)db->hdr.includes_std_num,
        sizeof(db->hdr.includes_std[0]),
        compare_includes
    );
    qsort(
        db->hdr_test.includes_std,
        (size_t)db->hdr_test.includes_std_num,
        sizeof(db->hdr_test.includes_std[0]),
        compare_includes
    );
}

/* generate a function name fn_name from file name */
static void convert_fname_to_fn_name(
    const char *fname, char fn_name[static PATH_MAX]
) {
    const char *src = fname;
    char *dst = fn_name;

    /* Skip the leading part until after "/amast/" */
    src = strstr(src, "/amast/");
    if (!src) {
        fn_name[0] = '\0';
        return;
    }
    /* Move the pointer to start with "amast/" */
    src += 1;
    while (*src) {
        if (*src == '/') {
            /* Replace '/' with '_' */
            *dst = '_';
        } else if (*src == '.') {
            break;
        } else {
            *dst = *src;
        }
        src++;
        dst++;
    }

    *dst = '\0';
}

static void file_append(
    char *src,
    const char *src_fname,
    FILE *dst,
    int *ntests,
    char (*tests)[PATH_MAX]
) {
    /*
     * There must be only one main() in the resulting file.
     * So, replace main() with a unique function name
     */
    const char *main_fn = "int main(void) {";
    char *pos = strstr(src, main_fn);
    if (!pos || !ntests || !tests) {
        fputs(src, dst);
        return;
    }
    char fn_name[PATH_MAX];
    convert_fname_to_fn_name(src_fname, fn_name);
    char buffer[2 * PATH_MAX];
    snprintf(buffer, sizeof(buffer), "static int %s(void) {\n", fn_name);
    str_lcpy(tests[*ntests], fn_name, sizeof(tests[*ntests]));
    (*ntests)++;
    *pos = '\0';
    fputs(src, dst);
    fputs(buffer, dst);
    pos += strlen(main_fn) + /*sizeof('\n')=*/1;
    fputs(pos, dst);
}

static const char *get_repo_fname(const char *fname) {
    const char *src = strstr(fname, "/amast/");
    AM_ASSERT(src);
    return src + 1;
}

static void add_amast_description(
    FILE *f, const char *note, const struct files *db
) {
    fprintf(f, "/*\n");
    fprintf(f, " * This file was auto-generated as a copy-paste\n");
    fprintf(f, " * combination of AMAST project %s files taken from\n", note);
    fprintf(f, " * GitHub repo https://github.com/adel-mamin/amast\n");
    fprintf(f, " * Version %s\n", AMAST_VERSION);
    fprintf(f, " */\n");
    fprintf(f, "\n");

    fprintf(f, "/*\n");
    fprintf(f, " * The complete list of the copy-pasted %s files:\n", note);
    fprintf(f, " *\n");
    for (int i = 0; i < db->len; ++i) {
        fprintf(f, " * %s\n", get_repo_fname(db->fnames[i]));
    }
    fprintf(f, " */\n");
    fprintf(f, "\n");
}

static void add_amast_includes_std(FILE *f, const struct files *db) {
    for (int i = 0; i < db->includes_std_num; ++i) {
        if (strstr(db->includes_std[i], "#include") ||
            /* verbatim inclusion */
            strstr(db->includes_std[i], "#define")) {
            fprintf(f, "%s", db->includes_std[i]);
        } else if (db->includes_std[i][0] == '\n') {
            fprintf(f, "\n");
        } else {
            fprintf(f, "#include <%s>\n", db->includes_std[i]);
        }
    }
    fprintf(f, "\n");
}

static void create_amast_h_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX]
) {
    char fname[PATH_MAX];
    snprintf(fname, sizeof(fname), "%s/amast.h", db->odir);
    FILE *hdr_file = fopen(fname, "w");
    if (!hdr_file) {
        fprintf(stderr, "Failed to create %s\n", fname);
        exit(EXIT_FAILURE);
    }

    fprintf(hdr_file, "#ifndef AMAST_H_INCLUDED\n");
    fprintf(hdr_file, "#define AMAST_H_INCLUDED\n");
    fprintf(hdr_file, "\n");

    add_amast_description(hdr_file, "header", &db->hdr);
    add_amast_includes_std(hdr_file, &db->hdr);

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#include \"amast_config.h\"\n");
    fprintf(hdr_file, "\n");

    /* Copy content of all header files to amast.h */
    for (int i = 0; i < db->hdr.len; i++) {
        fprintf(hdr_file, "\n/* %s */\n\n", get_repo_fname(db->hdr.fnames[i]));
        file_append(
            db->hdr.content[i], db->hdr.fnames[i], hdr_file, ntests, tests
        );
    }

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#endif /* AMAST_H_INCLUDED */\n");

    fclose(hdr_file);
}

static void create_amast_test_h_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX]
) {
    char fname[PATH_MAX];
    snprintf(fname, sizeof(fname), "%s/amast_test.h", db->odir);
    FILE *hdr_file = fopen(fname, "w");
    if (!hdr_file) {
        fprintf(stderr, "Failed to create %s\n", fname);
        exit(EXIT_FAILURE);
    }

    fprintf(hdr_file, "#ifndef AMAST_TEST_H_INCLUDED\n");
    fprintf(hdr_file, "#define AMAST_TEST_H_INCLUDED\n");
    fprintf(hdr_file, "\n");

    add_amast_description(hdr_file, "header", &db->hdr_test);
    add_amast_includes_std(hdr_file, &db->hdr_test);

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#include \"amast_config.h\"\n");
    fprintf(hdr_file, "\n");

    /* Copy content of all header files to amast.h */
    for (int i = 0; i < db->hdr_test.len; i++) {
        fprintf(
            hdr_file, "\n/* %s */\n\n", get_repo_fname(db->hdr_test.fnames[i])
        );
        file_append(
            db->hdr_test.content[i],
            db->hdr_test.fnames[i],
            hdr_file,
            ntests,
            tests
        );
    }

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#endif /* AMAST_TEST_H_INCLUDED */\n");

    fclose(hdr_file);
}

struct amast_file_cfg {
    struct db *db;
    int *ntests;
    char (*tests)[PATH_MAX];
    int tests_max;
    struct files *files;
    const char (*inc)[PATH_MAX];
    int ninc;
    const char *amast_fname;
    const char *note;
    bool keep_open;
};

static FILE *create_amast_file(struct amast_file_cfg *cfg) {
    char fname[PATH_MAX];
    snprintf(fname, sizeof(fname), "%s/%s", cfg->db->odir, cfg->amast_fname);
    FILE *src_file = fopen(fname, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to create %s\n", fname);
        exit(EXIT_FAILURE);
    }

    add_amast_description(src_file, cfg->note, cfg->files);

    add_amast_includes_std(src_file, cfg->files);
    for (int i = 0; i < cfg->ninc; ++i) {
        fprintf(src_file, "%s\n", cfg->inc[i]);
    }
    fprintf(src_file, "\n");

    /* Copy content of all source files to cfg->amast_fname */
    for (int i = 0; i < cfg->files->len; i++) {
        fprintf(
            src_file, "\n/* %s */\n\n", get_repo_fname(cfg->files->fnames[i])
        );
        AM_ASSERT(*cfg->ntests < cfg->tests_max);
        AM_ASSERT(strstr(cfg->db->src.fnames[i], "test") == NULL);
        file_append(
            cfg->files->content[i],
            cfg->files->fnames[i],
            src_file,
            cfg->ntests,
            cfg->tests
        );
    }

    if (cfg->keep_open) {
        return src_file;
    }
    fclose(src_file);
    return NULL;
}

static void create_amast_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"", "#include \"amast.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast.c",
        .note = "source"
    };
    create_amast_file(&cfg);
}

static void create_amast_freertos_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"", "#include \"amast.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src_freertos,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast_freertos.c",
        .note = "source"
    };
    create_amast_file(&cfg);
}

static void create_amast_posix_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"", "#include \"amast.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src_posix,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast_posix.c",
        .note = "source"
    };
    create_amast_file(&cfg);
}

static void create_amast_cooperative_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"", "#include \"amast.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src_cooperative,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast_cooperative.c",
        .note = "source"
    };
    create_amast_file(&cfg);
}

static void create_amast_preemptive_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"", "#include \"amast.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src_preemptive,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast_preemptive.c",
        .note = "source"
    };
    create_amast_file(&cfg);
}

static void create_amast_test_c_file(
    struct db *db, int *ntests, char (*tests)[PATH_MAX], int tests_max
) {
    static const char inc[][PATH_MAX] = {
        "#include \"amast_config.h\"",
        "#include \"amast.h\"",
        "#include \"amast_test.h\""
    };
    struct amast_file_cfg cfg = {
        .db = db,
        .ntests = ntests,
        .tests = tests,
        .tests_max = tests_max,
        .files = &db->src_test,
        .inc = inc,
        .ninc = AM_COUNTOF(inc),
        .amast_fname = "amast_test.c",
        .note = "source",
        .keep_open = true
    };
    FILE *src_file = create_amast_file(&cfg);

    /* Add the final main function to amast_test.c */
    fprintf(src_file, "\nint main(void) {\n");
    for (int i = 0; i < *ntests; i++) {
        fprintf(src_file, "    %s();\n", tests[i]);
    }
    fprintf(src_file, "\n");
    fprintf(src_file, "    printf(\"Amast unit tests passed!\\n\");\n");
    fprintf(src_file, "\n");
    fprintf(src_file, "    return 0;\n");
    fprintf(src_file, "}\n");
    fprintf(src_file, "\n");

    fclose(src_file);
}

static void create_amast_files(struct db *db) {
    char tests[32][PATH_MAX];
    int ntests = 0;

    create_amast_h_file(db, &ntests, tests);
    create_amast_test_h_file(db, &ntests, tests);

    create_amast_c_file(db, &ntests, tests, AM_COUNTOF(tests));
    create_amast_freertos_c_file(db, &ntests, tests, AM_COUNTOF(tests));
    create_amast_posix_c_file(db, &ntests, tests, AM_COUNTOF(tests));
    create_amast_cooperative_c_file(db, &ntests, tests, AM_COUNTOF(tests));
    create_amast_preemptive_c_file(db, &ntests, tests, AM_COUNTOF(tests));

    create_amast_test_c_file(db, &ntests, tests, AM_COUNTOF(tests));
}

static void print_help(const char *cmd) {
    printf("Usage: %s -f <file name> -o <output directory>\n", cmd);
    printf(
        "Creates amast(-test).h and amast(-test).c files from the list "
        "of files in <file name>\n"
    );
    printf("The files are created in the <output directory>\n");
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *fname = NULL;
    const char *odir = NULL;
    int opt = 0;

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "f:o:")) != -1) {
        switch (opt) {
        case 'f':
            fname = optarg;
            break;
        case 'o':
            odir = optarg;
            break;
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (fname == NULL || odir == NULL) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Generating amast(-test).h and amast(-test).c in %s ... ", odir);

    db_init(&m_db, fname, odir);
    create_amast_files(&m_db);

    printf("done.\n");
    return 0;
}
