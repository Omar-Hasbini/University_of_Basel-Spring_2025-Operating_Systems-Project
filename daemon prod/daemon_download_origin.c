/* daemon_download_origin.c */
#define _GNU_SOURCE           /* Enable GNU extensions */
#include <stdio.h>            /* Standard I/O routines */
#include <stdlib.h>           /* General utilities: getenv, malloc */
#include <string.h>           /* Memory and string functions */
#include <unistd.h>           /* POSIX API: access, usleep, getuid */
#include <sqlite3.h>          /* SQLite3 database functions */
#include <limits.h>           /* PATH_MAX constant */
#include <libgen.h>           /* basename() */
#include <dirent.h>           /* Directory traversal functions */
#include <errno.h>            /* errno and error codes */
#include <pwd.h>              /* getpwuid() to lookup passwd entry */
#include <sys/types.h>        /* Type definitions: uid_t */
#include <sys/xattr.h>        /* Extended attribute functions */
#include <sys/stat.h>         /* File status functions */

#ifndef ENOATTR
#define ENOATTR ENODATA       /* Define ENOATTR on systems lacking it */
#endif

/* Name of the extended attribute to store origin URL */
#define ORIGIN_ATTR      "user.origin"
/* Maximum path length for buffers */
#define MAX_PATH         1024
/* Number of retries when DB is busy/locked */
#define RETRY_LIMIT      20
/* Milliseconds to wait between retry attempts */
#define RETRY_WAIT_MS    100
/* Seconds to wait between scanning the Downloads directory */
#define SCAN_INTERVAL    5

/**
 * Sleep for the specified number of milliseconds.
 */
static void sleep_ms(int ms) {
    usleep(ms * 1000);
}

/**
 * Copy a SQLite database file and its WAL/SHM companions to a new location.
 * This allows safe, read-only access while Firefox may still be writing.
 *
 * @param src  Path to the original database file
 * @param dst  Path to the temporary copy
 * @return     1 on success, 0 on failure
 */
static int copy_sqlite_with_wal(const char *src, const char *dst) {
    /* Ensure we can read the source file */
    if (access(src, R_OK) != 0) {
        fprintf(stderr, "❌ Cannot access base DB: %s\n", src);
        return 0;
    }

    /* Copy the main DB file to the destination */
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", src, dst);
    if (system(cmd) != 0) {
        fprintf(stderr, "❌ Failed to copy base DB file\n");
        return 0;
    }

    /* Also copy the WAL and SHM files, if they exist */
    char wal_src[MAX_PATH], shm_src[MAX_PATH];
    char wal_dst[MAX_PATH], shm_dst[MAX_PATH];
    snprintf(wal_src, sizeof(wal_src), "%s-wal", src);
    snprintf(wal_dst, sizeof(wal_dst), "%s-wal", dst);
    if (access(wal_src, R_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", wal_src, wal_dst);
        system(cmd);
    }
    snprintf(shm_src, sizeof(shm_src), "%s-shm", src);
    snprintf(shm_dst, sizeof(shm_dst), "%s-shm", dst);
    if (access(shm_src, R_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", shm_src, shm_dst);
        system(cmd);
    }

    return 1;
}

/**
 * Search the in-memory SQLite database for a URL containing the given filename.
 * Scans the most recent 100 entries in moz_places.
 *
 * @param db        Open SQLite database handle
 * @param filename  Filename to search for in the URL
 * @return          Dynamically allocated URL on success, NULL if none found
 */
static char* get_url_for_filename(sqlite3 *db, const char *filename) {
    const char *sql =
        "SELECT url FROM moz_places "
        "ORDER BY last_visit_date DESC LIMIT 100;";
    sqlite3_stmt *stmt;

    /* Prepare the SQL statement */
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ sqlite prepare error: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    /* Iterate rows and look for the filename as a substring in the URL */
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *url = sqlite3_column_text(stmt, 0);
        if (url && strstr((const char*)url, filename)) {
            char *result = strdup((const char*)url);
            sqlite3_finalize(stmt);
            return result;
        }
    }

    sqlite3_finalize(stmt);
    return NULL;
}

/**
 * Load the Firefox places.sqlite database into memory (via backup API)
 * and search it for a URL matching the given filename.
 *
 * @param filename    Basename of the file in Downloads
 * @param places_src  Path to the on-disk places.sqlite file
 * @return            Dynamically allocated URL string or NULL if none
 */
static char* find_url_for_file(const char *filename, const char *places_src) {
    const char *tmpdb = "/tmp/places_copy.sqlite";
    if (!copy_sqlite_with_wal(places_src, tmpdb))
        return NULL;

    sqlite3 *src = NULL, *mem = NULL;
    /* Open the copied file on disk */
    if (sqlite3_open(tmpdb, &src) != SQLITE_OK) {
        fprintf(stderr, "❌ open copied DB: %s\n", sqlite3_errmsg(src));
        sqlite3_close(src);
        return NULL;
    }
    /* Open an in-memory database */
    if (sqlite3_open(":memory:", &mem) != SQLITE_OK) {
        fprintf(stderr, "❌ open memory DB: %s\n", sqlite3_errmsg(mem));
        sqlite3_close(src);
        return NULL;
    }

    /* Copy from disk to memory */
    sqlite3_backup *bk = sqlite3_backup_init(mem, "main", src, "main");
    if (!bk) {
        fprintf(stderr, "❌ sqlite backup init: %s\n", sqlite3_errmsg(mem));
        sqlite3_close(src);
        sqlite3_close(mem);
        return NULL;
    }

    /* Perform the backup with retry logic */
    int retries = 0, rc;
    do {
        rc = sqlite3_backup_step(bk, -1);
        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
            sleep_ms(RETRY_WAIT_MS);
    } while ((rc == SQLITE_BUSY || rc == SQLITE_LOCKED) && retries++ < RETRY_LIMIT);
    sqlite3_backup_finish(bk);
    sqlite3_close(src);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ backup failed after %d retries: %d\n", retries, rc);
        sqlite3_close(mem);
        return NULL;
    }

    /* Search for the URL in the in-memory DB */
    char *url = get_url_for_filename(mem, filename);
    sqlite3_close(mem);
    return url;
}

int main(void) {
    /* Determine the real user's home directory, even under sudo */
    const char *sudo_uid = getenv("SUDO_UID");
    uid_t uid = sudo_uid ? (uid_t)atoi(sudo_uid) : getuid();
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        fprintf(stderr, "❌ getpwuid(%d): %s\n", uid, strerror(errno));
        return 1;
    }
    const char *home = pw->pw_dir;

    /* Construct the path to the user's Downloads directory */
    char downloads_dir[MAX_PATH];
    snprintf(downloads_dir, sizeof(downloads_dir), "%s/Downloads", home);

    /* Build a find command to locate places.sqlite in snap or .mozilla */
    char find_cmd[MAX_PATH*2];
    snprintf(find_cmd, sizeof(find_cmd),
        "find '%s/snap/firefox' '%s/.mozilla/firefox' -type f -name places.sqlite | head -n1",
        home, home);

    /* Execute the find command */
    FILE *fp = popen(find_cmd, "r");
    if (!fp) {
        fprintf(stderr, "❌ popen(find): %s\n", strerror(errno));
        return 1;
    }

    /* Read the resulting path to places.sqlite */
    char places_src[MAX_PATH];
    if (!fgets(places_src, sizeof(places_src), fp)) {
        fprintf(stderr, "❌ Failed to locate places.sqlite using:\n  %s\n", find_cmd);
        pclose(fp);
        return 1;
    }
    pclose(fp);
    places_src[strcspn(places_src, "\n")] = '\0';

    printf("✅ Watching %s, using DB at %s\n", downloads_dir, places_src);

    /* Main loop: scan Downloads directory periodically */
    while (1) {
        DIR *d = opendir(downloads_dir);
        if (!d) { perror("opendir"); sleep(SCAN_INTERVAL); continue; }
        struct dirent *entry;
        while ((entry = readdir(d))) {
            /* Skip hidden entries "." and ".." */
            if (entry->d_name[0] == '.')
                continue;

            /* Build full file path */
            char filepath[MAX_PATH];
            snprintf(filepath, sizeof(filepath), "%s/%s", downloads_dir, entry->d_name);

            /* Only process regular files */
            struct stat st;
            if (stat(filepath, &st) == -1 || !S_ISREG(st.st_mode))
                continue;

            /* Skip files that already have the origin attribute */
            char buf[MAX_PATH];
            ssize_t s = getxattr(filepath, ORIGIN_ATTR, buf, sizeof(buf));
            if (s >= 0)          /* Attribute exists */
                continue;
            if (errno != ENODATA && errno != ENOATTR)
                continue;

            /* Prepare filename for URL lookup by stripping any "(n)" suffix */
            char cleanname[MAX_PATH];
            strncpy(cleanname, entry->d_name, MAX_PATH);
            cleanname[MAX_PATH-1] = '\0';
            char *p = strchr(cleanname, '(');
            if (p) 
                *p = '\0';

            /* Lookup origin URL from Firefox history using cleaned name */
            char *url = find_url_for_file(cleanname, places_src);
            if (url) {
                /* Attach the URL as an extended attribute */
                if (setxattr(filepath, ORIGIN_ATTR, url, strlen(url), 0) == 0)
                    printf("✅ %s → %s\n", filepath, url);
                else
                    fprintf(stderr, "❌ setxattr on %s: %s\n", filepath, strerror(errno));
                free(url);
            }
        }
        closedir(d);
        sleep(SCAN_INTERVAL);
    }

    return 0;
}

