/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <linux/limits.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/common/macros.h"
#include "libs/blk/blk.h"

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
    struct files hdr;
    const char *odir; /* amast.h and amast.c are placed here */
};

static struct db m_db = {.src.len = 0, .hdr.len = 0};

/* check if the include already exists in the array */
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

/* add unique include to the array */
static void include_add_unique(
    char arr[MAX_INCLUDES_NUM][PATH_MAX], int *arr_size, const char *inc_file
) {
    if (include_is_unique(arr, *arr_size, inc_file)) {
        strcpy(arr[*arr_size], inc_file);
        (*arr_size)++;
    }
}

/* process a line and detect #include directives */
static void process_content(struct files *db, char *line) {
    char inc_file[PATH_MAX];
    if (sscanf(line, "#include <%[^>]>%*s", inc_file) == 1) {
        include_add_unique(db->includes_std, &db->includes_std_num, inc_file);
    } else if (sscanf(line, "#include \"%[^\"]\"%*s", inc_file) == 1) {
        /* ignore user includes */;
    } else { /* non-include line */
        strcat(db->content[db->len], line);
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
    while (fgets(line, sizeof(line), file)) {
        process_content(db, line);
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
            assert(db->src.len < AM_COUNTOF(db->src.content));
            read_file(&db->src, fname);
            db->src.len++;
            continue;
        }
        if (strstr(fname, ".h") != NULL) {
            assert(db->hdr.len < AM_COUNTOF(db->hdr.content));
            read_file(&db->hdr, fname);
            db->hdr.len++;
            continue;
        }
        assert(0);
    }

    fclose(file);
    db->odir = odir;

    qsort(
        db->src.includes_std,
        db->src.includes_std_num,
        sizeof(db->src.includes_std[0]),
        compare_includes
    );
    qsort(
        db->hdr.includes_std,
        db->hdr.includes_std_num,
        sizeof(db->hdr.includes_std[0]),
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
     * There must only be one main() in the resulting file.
     * So, replace main() with a unique function name
     */
    const char *main_fn = "int main(void) {";
    char *pos = strstr(src, main_fn);
    if (!pos) {
        fputs(src, dst);
        return;
    }
    char fn_name[PATH_MAX];
    convert_fname_to_fn_name(src_fname, fn_name);
    char buffer[2 * PATH_MAX];
    snprintf(buffer, sizeof(buffer), "int %s(void) {\n", fn_name);
    strcpy(tests[*ntests], fn_name);
    (*ntests)++;
    *pos = '\0';
    fputs(src, dst);
    fputs(buffer, dst);
    pos += strlen(main_fn) + /*sizeof('\n')=*/1;
    fputs(pos, dst);
}

const char *get_repo_fname(const char *fname) {
    const char *src = strstr(fname, "/amast/");
    assert(src);
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

static void add_amast_includes_std(FILE *f, struct files *db) {
    for (int i = 0; i < db->includes_std_num; ++i) {
        fprintf(f, "#include <%s>\n", db->includes_std[i]);
    }
    fprintf(f, "\n");
}

static void create_amast_files(struct db *db) {
    char amast_h[PATH_MAX];
    snprintf(amast_h, sizeof(amast_h), "%s/amast.h", db->odir);
    FILE *hdr_file = fopen(amast_h, "w");
    if (!hdr_file) {
        fprintf(stderr, "Failed to create %s\n", amast_h);
        exit(EXIT_FAILURE);
    }
    char amast_c[PATH_MAX];
    snprintf(amast_c, sizeof(amast_c), "%s/amast.c", db->odir);
    FILE *src_file = fopen(amast_c, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to create %s\n", amast_c);
        fclose(hdr_file);
        exit(EXIT_FAILURE);
    }

    fprintf(hdr_file, "#ifndef AMAST_H_INCLUDED\n");
    fprintf(hdr_file, "#define AMAST_H_INCLUDED\n");
    fprintf(hdr_file, "\n");

    add_amast_description(hdr_file, "header", &m_db.hdr);
    add_amast_includes_std(hdr_file, &m_db.hdr);

    fprintf(hdr_file, "#ifdef AMAST_UNIT_TESTS\n");
    fprintf(hdr_file, "#undef AM_HSM_SPY\n");
    fprintf(hdr_file, "#define AM_HSM_SPY\n");
    fprintf(hdr_file, "#endif /* AMAST_UNIT_TESTS */ \n");

    char tests[32][PATH_MAX];
    int ntests = 0;
    /* Copy content of all header files to amast.h */
    for (int i = 0; i < db->hdr.len; i++) {
        assert(ntests < (AM_COUNTOF(tests) - 1));
        fprintf(hdr_file, "\n/* %s */\n\n", get_repo_fname(db->hdr.fnames[i]));
        file_append(
            db->hdr.content[i], db->hdr.fnames[i], hdr_file, &ntests, tests
        );
    }

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#endif /* AMAST_H_INCLUDED */\n");

    add_amast_description(src_file, "source", &m_db.src);

    add_amast_includes_std(src_file, &m_db.src);
    fprintf(src_file, "#include \"amast.h\"\n");
    fprintf(src_file, "\n");

    /* Copy content of all source files to amast.c */
    for (int i = 0; i < db->src.len; i++) {
        fprintf(src_file, "/* %s */\n", get_repo_fname(db->src.fnames[i]));
        assert(ntests < (AM_COUNTOF(tests) - 1));
        if (strstr(db->src.fnames[i], "test") != NULL) {
            fprintf(src_file, "\n");
            fprintf(src_file, "#ifdef AMAST_UNIT_TESTS\n");
            fprintf(src_file, "\n");
            file_append(
                db->src.content[i], db->src.fnames[i], src_file, &ntests, tests
            );
            fprintf(src_file, "\n");
            fprintf(src_file, "#endif /* AMAST_UNIT_TESTS */\n");
            fprintf(src_file, "\n");
            continue;
        }
        file_append(
            db->src.content[i], db->src.fnames[i], src_file, &ntests, tests
        );
    }

    /* Add the final main function to amast.c */
    fprintf(src_file, "\n");
    fprintf(src_file, "#ifdef AMAST_UNIT_TESTS\n");
    fprintf(src_file, "\n");
    fprintf(src_file, "int main(void) {\n");
    for (int i = 0; i < ntests; i++) {
        fprintf(src_file, "    %s();\n", tests[i]);
    }
    fprintf(src_file, "    return 0;\n");
    fprintf(src_file, "}\n");
    fprintf(src_file, "\n");
    fprintf(src_file, "#endif /* AMAST_UNIT_TESTS */\n");

    fclose(hdr_file);
    fclose(src_file);
}

static void print_help(const char *cmd) {
    printf("Usage: %s -f <file name> -o <output directory>\n", cmd);
    printf(
        "Creates amast.h and amast.c files from the list of files "
        "in <file name>\n"
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

    printf("Generating amast.h and amast.c in %s ... ", odir);

    db_init(&m_db, fname, odir);
    create_amast_files(&m_db);

    printf("done.\n");
    return 0;
}
