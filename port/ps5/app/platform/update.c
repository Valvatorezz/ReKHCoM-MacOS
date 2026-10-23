/*
 * ReKHCoM for PS5 - updates from the project's GitHub releases.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Checks api.github.com for the newest project release carrying PPSA98511.zip; installing downloads that asset and writes the
 * PPSA98511 folder inside it over the installed app. Each file goes to <name>.new and is renamed into place: a
 * running eboot.bin can't be overwritten, but it can be replaced by name, and the new one runs from the next start.
 * eboot.bin and param.json go last, so an interrupted update never leaves a new executable without its files.
 */
#include "update.h"

#include <curl/curl.h>
#include <dirent.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ReKHCoM.h"
#include "console_curl.h"

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../../third_party/miniz/miniz.h"

#define TITLE_ID ReKHCoM_TITLE_ID
#define DATA_ROOT "/data/ReKHCom"
#define APP_DIR_DEFAULT "/data/homebrew/" TITLE_ID
#define PATH_MAX_LEN 512
#define USER_AGENT "ReKHCoM-PS5/" ReKHCoM_VERSION

static volatile int sState;
static char sVersion[32], sZipUrl[512], sMessage[256];
static void (*sLog)(const char *fmt, ...);

#define LOG(...)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (sLog)                                                                                                      \
            sLog(__VA_ARGS__);                                                                                         \
    } while (0)

static void set_state(int s)
{
    __atomic_store_n(&sState, s, __ATOMIC_SEQ_CST);
}

int update_state(void)
{
    return __atomic_load_n(&sState, __ATOMIC_SEQ_CST);
}

const char *update_version(void)
{
    return sVersion;
}

const char *update_message(void)
{
    return sMessage;
}

void update_init(void (*log)(const char *fmt, ...))
{
    sLog = log;
}

static void fail(const char *why)
{
    snprintf(sMessage, sizeof(sMessage), "%s", why);
    LOG("update: %s", why);
    set_state(UPDATE_FAILED);
}

/* "1.10.0" > "1.9.2" */
static int newer(const char *a, const char *b)
{
    int x[3] = {0}, y[3] = {0};
    sscanf(a, "%d.%d.%d", &x[0], &x[1], &x[2]);
    sscanf(b, "%d.%d.%d", &y[0], &y[1], &y[2]);
    for (int i = 0; i < 3; ++i)
        if (x[i] != y[i])
            return x[i] > y[i];
    return 0;
}

/* the value of "key": "..." after `from` in a JSON text */
static int json_string(const char *from, const char *key, char *out, size_t size)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(from, pattern);
    if (!p)
        return 0;
    p = strchr(p + strlen(pattern), '"');
    if (!p)
        return 0;
    const char *end = strchr(++p, '"');
    if (!end || (size_t)(end - p) >= size)
        return 0;
    memcpy(out, p, (size_t)(end - p));
    out[end - p] = '\0';
    return 1;
}

/* ---- HTTP ----------------------------------------------------------------------------------------------------- */

typedef struct
{
    char *data;
    size_t length, capacity;
} Buffer;

static size_t buffer_cb(void *data, size_t size, size_t count, void *user)
{
    Buffer *b = user;
    size_t n = size * count;
    if (b->length + n + 1 > b->capacity)
    {
        size_t cap = (b->length + n + 1) * 2;
        char *grown = realloc(b->data, cap);
        if (!grown)
            return 0;
        b->data = grown;
        b->capacity = cap;
    }
    memcpy(b->data + b->length, data, n);
    b->length += n;
    b->data[b->length] = '\0';
    return n;
}

static size_t file_cb(void *data, size_t size, size_t count, void *user)
{
    return fwrite(data, size, count, (FILE *)user) * size;
}

static int curl_ready(void)
{
    static int state = -1;
    if (state < 0)
        state = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    return state == 1;
}

static CURL *easy_new(const char *url)
{
    CURL *easy = curl_easy_init();
    if (!easy)
        return NULL;
    console_curl_setup(easy);
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, USER_AGENT);
    return easy;
}

/* GET: the body comes back in *body (malloc'd, NUL-terminated). The HTTP status, or -1 when it could not be made. */
static int http_get(const char *url, char **body)
{
    Buffer b = {0};
    CURL *easy;
    long status = 0;
    CURLcode rc;

    *body = NULL;
    if (!curl_ready() || !(easy = easy_new(url)))
        return -1;
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 25L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, buffer_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &b);
    rc = curl_easy_perform(easy);
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(easy);
    if (rc != CURLE_OK)
    {
        LOG("update: %s -> curl %d", url, (int)rc);
        free(b.data);
        return -1;
    }
    *body = b.data;
    return (int)status;
}

/* Downloads to dest (written to dest.part, then renamed). No total time limit: the zip is tens of MB; it gives up
   when the speed stays under 1 KB/s for 30 seconds. */
static int http_download(const char *url, const char *dest)
{
    char part[PATH_MAX_LEN + 8];
    FILE *f;
    CURL *easy;
    long status = 0;
    CURLcode rc;
    int written, ok;

    if (!curl_ready())
        return 0;
    snprintf(part, sizeof(part), "%s.part", dest);
    if (!(f = fopen(part, "wb")))
        return 0;
    if (!(easy = easy_new(url)))
    {
        fclose(f);
        remove(part);
        return 0;
    }
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, file_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, f);
    rc = curl_easy_perform(easy);
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(easy);
    written = fclose(f) == 0;
    ok = rc == CURLE_OK && status == 200 && written && rename(part, dest) == 0;
    if (!ok)
    {
        LOG("update: %s -> curl %d, http %ld", url, (int)rc, status);
        remove(part);
    }
    return ok;
}

/* ---- check ---------------------------------------------------------------------------------------------------- */

static void *check_thread(void *unused)
{
    char *body = NULL;
    int status, available;
    (void)unused;

    /* every recent release, not /releases/latest: the PC releases of the project are not PS5 builds */
    status = http_get("https://api.github.com/repos/" UPDATE_REPO "/releases?per_page=30", &body);
    if (status == 404)
    {
        free(body);
        fail("No release found (the repository may be private)");
        return NULL;
    }
    if (status != 200 || !body)
    {
        free(body);
        fail("Couldn't reach GitHub");
        return NULL;
    }
    /* the newest vX.Y.Z release (not a draft or pre-release) carrying the PS5 package */
    sVersion[0] = '\0';
    sZipUrl[0] = '\0';
    for (const char *rel = strstr(body, "\"tag_name\""); rel;)
    {
        const char *next = strstr(rel + 10, "\"tag_name\"");
        size_t span = next ? (size_t)(next - rel) : strlen(rel);
        char *one = malloc(span + 1);
        char tag[48] = "", url[512] = "", found[512] = "";
        int skip;
        const char *tv;

        if (!one)
            break;
        memcpy(one, rel, span);
        one[span] = '\0';
        json_string(one, "tag_name", tag, sizeof(tag));
        skip = strncmp(tag, UPDATE_TAG_PREFIX, strlen(UPDATE_TAG_PREFIX)) != 0 || strstr(one, "\"draft\":true") ||
               strstr(one, "\"prerelease\":true");
        for (const char *p = one; !skip && (p = strstr(p, "\"browser_download_url\""));)
        {
            if (json_string(p, "browser_download_url", url, sizeof(url)))
            {
                const char *asset = strrchr(url, '/');
                if (asset && !strcmp(asset + 1, "PPSA98511.zip"))
                {
                    snprintf(found, sizeof(found), "%s", url);
                    break;
                }
            }
            p += 22;
        }
        free(one);
        tv = tag + strlen(UPDATE_TAG_PREFIX);
        if (!skip && tv[0] && found[0] && (!sVersion[0] || newer(tv, sVersion)))
        {
            snprintf(sVersion, sizeof(sVersion), "%s", tv);
            snprintf(sZipUrl, sizeof(sZipUrl), "%s", found);
        }
        rel = next;
    }
    free(body);
    available = sVersion[0] && newer(sVersion, ReKHCoM_VERSION) && sZipUrl[0];
    LOG("update: latest %s, this %s%s", sVersion[0] ? sVersion : "?", ReKHCoM_VERSION,
        available ? ", available" : "");
    set_state(available ? UPDATE_AVAILABLE : UPDATE_NONE);
    return NULL;
}

static int start_thread(void *(*fn)(void *))
{
    pthread_t t;
    pthread_attr_t attr;
    int rc;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create(&t, &attr, fn, NULL);
    pthread_attr_destroy(&attr);
    return rc == 0;
}

void update_check(void)
{
    int s = update_state();
    if (s == UPDATE_CHECKING || s == UPDATE_INSTALLING || s == UPDATE_INSTALLED)
        return;
    set_state(UPDATE_CHECKING);
    if (!start_thread(check_thread))
        fail("no thread");
}

/* ---- install -------------------------------------------------------------------------------------------------- */

/* Where the running app is stored. The update has to land there, whether it's in /data/homebrew or on an extended
   or USB drive (/mnt/ext1/homebrew/PPSA98511 ...). The mount the app runs from (/app0) is matched to a folder on
   disk by its eboot.bin: same size and modification time. */
static int same_eboot(const char *dir, const struct stat *running)
{
    char file[PATH_MAX_LEN];
    struct stat st;
    snprintf(file, sizeof(file), "%s/eboot.bin", dir);
    return stat(file, &st) == 0 && st.st_size == running->st_size && st.st_mtime == running->st_mtime;
}

static const char *app_dir(void)
{
    static char found[PATH_MAX_LEN];
    static int done;
    static const char *const roots[] = {"/data",     "/mnt/ext0", "/mnt/ext1", "/mnt/usb0", "/mnt/usb1",
                                        "/mnt/usb2", "/mnt/usb3", "/mnt/usb4", "/mnt/usb5", "/mnt/usb6",
                                        "/mnt/usb7"};
    struct stat running;
    char file[PATH_MAX_LEN];

    if (done)
        return found;
    done = 1;
    snprintf(found, sizeof(found), "%s", APP_DIR_DEFAULT);
    snprintf(file, sizeof(file), "/app0/eboot.bin");
    if (stat(file, &running) != 0)
        return found;
    for (size_t r = 0; r < sizeof(roots) / sizeof(roots[0]); ++r)
    {
        char dir[PATH_MAX_LEN];
        DIR *d;
        struct dirent *e;

        /* <root>/<id> and <root>/<any folder>/<id>, such as <root>/homebrew/<id> */
        snprintf(dir, sizeof(dir), "%s/%s", roots[r], TITLE_ID);
        if (same_eboot(dir, &running))
        {
            snprintf(found, sizeof(found), "%s", dir);
            LOG("update: the app is in %s", found);
            return found;
        }
        if (!(d = opendir(roots[r])))
            continue;
        while ((e = readdir(d)) != NULL)
        {
            if (e->d_name[0] == '.')
                continue;
            snprintf(dir, sizeof(dir), "%s/%s/%s", roots[r], e->d_name, TITLE_ID);
            if (same_eboot(dir, &running))
            {
                snprintf(found, sizeof(found), "%s", dir);
                closedir(d);
                LOG("update: the app is in %s", found);
                return found;
            }
        }
        closedir(d);
    }
    LOG("update: couldn't find where the app is stored, using %s", found);
    return found;
}

/* The path of an entry inside the app folder of the zip ("PPSA98511/<path>"), or NULL. */
static const char *entry_path(const char *name)
{
    const char *inside = strstr(name, TITLE_ID "/");
    if (!inside || strstr(inside, ".."))
        return NULL;
    inside += strlen(TITLE_ID "/");
    return inside[0] ? inside : NULL;
}

/* eboot.bin and param.json are written last */
static int is_last(const char *path)
{
    return !strcmp(path, "eboot.bin") || !strcmp(path, "sce_sys/param.json");
}

static int extract(mz_zip_archive *archive, mz_uint index, const char *inside)
{
    char target[PATH_MAX_LEN], temp[PATH_MAX_LEN + 8], parent[PATH_MAX_LEN];
    char *slash;
    int ok;

    snprintf(target, sizeof(target), "%s/%s", app_dir(), inside);
    snprintf(parent, sizeof(parent), "%s", target);
    if ((slash = strrchr(parent, '/')) != NULL)
    {
        *slash = '\0';
        make_dirs(parent);
    }
    snprintf(temp, sizeof(temp), "%s.new", target);
    /* executable, as an FTP upload makes them: without it the PS5 won't start the app */
    ok = mz_zip_reader_extract_to_file(archive, index, temp, 0) && chmod(temp, 0777) == 0 && rename(temp, target) == 0;
    if (!ok)
        remove(temp);
    return ok;
}

static void *install_thread(void *unused)
{
    char zip[PATH_MAX_LEN], dir[PATH_MAX_LEN];
    mz_zip_archive archive;
    mz_uint count;
    int files = 0, ok = 1, hasEboot = 0, hasParam = 0;
    (void)unused;

    snprintf(dir, sizeof(dir), "%s/cache", DATA_ROOT);
    snprintf(zip, sizeof(zip), "%s/update.zip", dir);
    make_dirs(dir);
    if (!http_download(sZipUrl, zip))
    {
        fail("The download failed");
        return NULL;
    }
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_file(&archive, zip, 0))
    {
        remove(zip);
        fail("The download isn't a valid zip");
        return NULL;
    }
    count = mz_zip_reader_get_num_files(&archive);
    /* nothing is written unless the zip holds the whole app */
    for (mz_uint i = 0; i < count; ++i)
    {
        mz_zip_archive_file_stat st;
        const char *inside;
        if (!mz_zip_reader_file_stat(&archive, i, &st) || st.m_is_directory || !(inside = entry_path(st.m_filename)))
            continue;
        hasEboot |= !strcmp(inside, "eboot.bin");
        hasParam |= !strcmp(inside, "sce_sys/param.json");
    }
    if (!hasEboot || !hasParam)
    {
        mz_zip_reader_end(&archive);
        remove(zip);
        fail("The download isn't the app");
        return NULL;
    }
    for (int last = 0; last < 2 && ok; ++last)
        for (mz_uint i = 0; i < count && ok; ++i)
        {
            mz_zip_archive_file_stat st;
            const char *inside;
            if (!mz_zip_reader_file_stat(&archive, i, &st) || st.m_is_directory ||
                !(inside = entry_path(st.m_filename)) || is_last(inside) != last)
                continue;
            ok = extract(&archive, i, inside);
            files += ok;
        }
    mz_zip_reader_end(&archive);
    remove(zip);
    if (!ok || files == 0)
    {
        fail("Couldn't write the new files (is /data unlocked?)");
        return NULL;
    }
    snprintf(sMessage, sizeof(sMessage), "ReKHCoM %s is installed: restart the app to use it", sVersion);
    LOG("update: installed %s (%d files)", sVersion, files);
    set_state(UPDATE_INSTALLED);
    return NULL;
}

void update_install(void)
{
    if (update_state() != UPDATE_AVAILABLE)
        return;
    set_state(UPDATE_INSTALLING);
    if (!start_thread(install_thread))
        fail("no thread");
}
