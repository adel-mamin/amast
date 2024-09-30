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
#include <string.h>
#include <linux/limits.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/common/macros.h"
#include "libs/blk/blk.h"

#define DB_FILES_MAX 256

struct db {
    struct src {
        char names[DB_FILES_MAX][PATH_MAX];
        int len;
    } src;
    struct hdr {
        char names[DB_FILES_MAX][PATH_MAX];
        int len;
    } hdr;
    const char *odir; /* amast.h and amast.c are placed here */
};

static struct db m_db = {.src.len = 0, .hdr.len = 0};

static void db_init(struct db *db, const char *fname, const char *odir) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        fprintf(stderr, "Error: failed to open %s\n", fname);
        exit(EXIT_FAILURE);
    }

    char line[PATH_MAX];
    while (fgets(line, sizeof(line), file)) {
        /* Remove trailing newline if present */
        line[strcspn(line, "\n")] = 0;

        if (strstr(line, ".c") != NULL) {
            assert(db->src.len < DB_FILES_MAX);
            strncpy(db->src.names[db->src.len], line, PATH_MAX - 1);
            db->src.len++;
            continue;
        }
        if (strstr(line, ".h") != NULL) {
            assert(db->hdr.len < DB_FILES_MAX);
            strncpy(db->hdr.names[db->hdr.len], line, PATH_MAX - 1);
            db->hdr.len++;
            continue;
        }
        assert(0);
    }

    fclose(file);
    db->odir = odir;
}

void convert_fpath_to_name(const char *fpath, char name[static PATH_MAX]) {
    const char *src = fpath;
    char *dst = name;

    /* Skip the leading part until after "/amast/" */
    src = strstr(src, "/amast/");
    if (!src) {
        name[0] = '\0';
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

int file_append(
    const char *src, FILE *dst, int *ntests, char (*tests)[PATH_MAX]
) {
    FILE *src_file = fopen(src, "r");
    if (!src_file) {
        fprintf(stderr, "Error: Failed to open %s\n", src);
        return -1;
    }

    char buffer[8 * 1024];
    while (fgets(buffer, sizeof(buffer), src_file)) {
        char *custom_inc = strstr(buffer, "#include \"");
        if (custom_inc) {
            continue;
        }
        char *main_func = strstr(buffer, "int main(void) {");

        if (main_func) {
            char test_name[PATH_MAX];
            convert_fpath_to_name(src, test_name);
            snprintf(buffer, sizeof(buffer), "int %s(void) {\n", test_name);
            strcpy(tests[*ntests], test_name);
            (*ntests)++;
        }

        fputs(buffer, dst);
    }

    fclose(src_file);
    return 0;
}

const char *get_full_src_fname(const char *fname) {
    const char *src = strstr(fname, "/amast/");
    assert(src);
    return src + 1;
}

static void create_amast_files(const struct db *db) {
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

    char tests[32][PATH_MAX];
    int ntests = 0;
    /* Copy content of all header files to amast.h */
    for (int i = 0; i < db->hdr.len; i++) {
        assert(ntests < (AM_COUNTOF(tests) - 1));
        fprintf(
            hdr_file, "\n/* %s */\n\n", get_full_src_fname(db->hdr.names[i])
        );
        if (file_append(db->hdr.names[i], hdr_file, &ntests, tests) != 0) {
            fclose(hdr_file);
            fclose(src_file);
            exit(EXIT_FAILURE);
        }
    }

    fprintf(hdr_file, "\n");
    fprintf(hdr_file, "#endif /* AMAST_H_INCLUDED */\n");

    fprintf(src_file, "#include \"amast.h\"\n");
    fprintf(src_file, "\n");

    /* Copy content of all source files to amast.c */
    for (int i = 0; i < db->src.len; i++) {
        fprintf(src_file, "/* %s */\n", get_full_src_fname(db->src.names[i]));
        assert(ntests < (AM_COUNTOF(tests) - 1));
        if (strstr(db->src.names[i], "test") != NULL) {
            fprintf(src_file, "\n");
            fprintf(src_file, "#ifdef AMAST_UNIT_TESTS\n");
            fprintf(src_file, "\n");
            if (file_append(db->src.names[i], src_file, &ntests, tests) != 0) {
                fclose(hdr_file);
                fclose(src_file);
                exit(EXIT_FAILURE);
            }
            fprintf(src_file, "\n");
            fprintf(src_file, "#endif /* AMAST_UNIT_TESTS */\n");
            fprintf(src_file, "\n");
            continue;
        }
        if (file_append(db->src.names[i], src_file, &ntests, tests) != 0) {
            fclose(hdr_file);
            fclose(src_file);
            exit(EXIT_FAILURE);
        }
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
    printf("Creates amast.h and amast.c files from the list of files\n");
    printf("in <file name>\n");
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
