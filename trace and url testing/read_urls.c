#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <limits.h>
#include <libgen.h>

#define MAX_PATH 1024
#define RETRY_LIMIT 20
#define RETRY_WAIT_MS 100

void sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

int copy_sqlite_with_wal(const char *src, const char *dst) {
    if (access(src, R_OK) != 0) {
        fprintf(stderr, "❌ Cannot access base DB: %s\n", src);
        return 0;
    }

    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", src, dst);
    if (system(cmd) != 0) {
        fprintf(stderr, "❌ Failed to copy base DB file\n");
        return 0;
    }

    char wal_src[MAX_PATH], shm_src[MAX_PATH];
    char wal_dst[MAX_PATH], shm_dst[MAX_PATH];

    snprintf(wal_src, sizeof(wal_src), "%s-wal", src);
    snprintf(wal_dst, sizeof(wal_dst), "%s-wal", dst);
    if (access(wal_src, R_OK) == 0) {
        char wal_cmd[MAX_PATH * 2];
        snprintf(wal_cmd, sizeof(wal_cmd), "cp \"%s\" \"%s\"", wal_src, wal_dst);
        system(wal_cmd);
    }

    snprintf(shm_src, sizeof(shm_src), "%s-shm", src);
    snprintf(shm_dst, sizeof(shm_dst), "%s-shm", dst);
    if (access(shm_src, R_OK) == 0) {
        char shm_cmd[MAX_PATH * 2];
        snprintf(shm_cmd, sizeof(shm_cmd), "cp \"%s\" \"%s\"", shm_src, shm_dst);
        system(shm_cmd);
    }

    return 1;
}

char* get_url_for_filename(sqlite3 *db, const char *filename) {
    const char *sql =
        "SELECT url FROM moz_places "
        "ORDER BY last_visit_date DESC LIMIT 100;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *url = sqlite3_column_text(stmt, 0);
        if (url && strstr((const char *)url, filename)) {
            char *result = strdup((const char *)url);
            sqlite3_finalize(stmt);
            return result;
        }
    }

    sqlite3_finalize(stmt);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *target_filename = basename(argv[1]);

    char dbpath[MAX_PATH];
    char tmpdb[MAX_PATH] = "/tmp/places_copy.sqlite";

    FILE *fp = popen("find ~/snap/firefox -name places.sqlite | head -n 1", "r");
    if (!fp || !fgets(dbpath, sizeof(dbpath), fp)) {
        fprintf(stderr, "❌ Failed to locate places.sqlite\n");
        return 1;
    }
    pclose(fp);
    dbpath[strcspn(dbpath, "\n")] = '\0';

    if (!copy_sqlite_with_wal(dbpath, tmpdb)) {
        fprintf(stderr, "❌ Failed to copy places.sqlite and -wal/-shm files\n");
        return 1;
    }

    sqlite3 *src, *mem;
    if (sqlite3_open(tmpdb, &src) != SQLITE_OK) {
        fprintf(stderr, "❌ Failed to open copied DB\n");
        return 1;
    }

    if (sqlite3_open(":memory:", &mem) != SQLITE_OK) {
        fprintf(stderr, "❌ Failed to open in-memory DB\n");
        sqlite3_close(src);
        return 1;
    }

    sqlite3_backup *backup = sqlite3_backup_init(mem, "main", src, "main");
    if (!backup) {
        fprintf(stderr, "❌ Failed to initialize backup: %s\n", sqlite3_errmsg(mem));
        sqlite3_close(src);
        sqlite3_close(mem);
        return 1;
    }

    int retries = 0, rc;
    do {
        rc = sqlite3_backup_step(backup, -1);
        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
            sleep_ms(RETRY_WAIT_MS);
    } while ((rc == SQLITE_BUSY || rc == SQLITE_LOCKED) && retries++ < RETRY_LIMIT);

    sqlite3_backup_finish(backup);
    sqlite3_close(src);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ Backup step failed after %d retries: %d (%s)\n", retries, rc, sqlite3_errmsg(mem));
        sqlite3_close(mem);
        return 1;
    }

    char *matched_url = get_url_for_filename(mem, target_filename);
    if (matched_url) {
        printf("✅ Matched URL for '%s':\n%s\n", target_filename, matched_url);
        free(matched_url);
    } else {
        printf("⚠️  No matching URL found for '%s'.\n", target_filename);
    }

    sqlite3_close(mem);
    return 0;
}

