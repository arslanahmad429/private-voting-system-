/* ================================================================
 *  ONLINE VOTING SYSTEM - CONSOLIDATED SERVER
 *  Developed by: Anas Farooq
 *  BS Computer Science - Ziauddin University
 *
 *  All backend code merged into a single file.
 *  Libraries: mongoose.c (HTTP), sqlite3.c (Database)
 * ================================================================ */

#include "mongoose.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* Forward declaration */
void export_all_data_xls(void);

/* Forward declarations for export functions */
void export_voters_xls(void);
void export_elections_xls(void);
void export_votes_xls(void);
void export_candidates_xls(void);
void export_applications_xls(void);
/* ================================================================
   SHA-256 HASHING
   ================================================================ */



#define SHA256_BLOCK_SIZE 32

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

// Helper to hash a string to hex 
void sha256_string(const char *string, char outputBuffer[65]);


/* ================================================================
   SHA-256 IMPLEMENTATION
   ================================================================ */


#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
	uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx) {
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
	uint32_t i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
	uint32_t i;

	i = ctx->datalen;
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

void sha256_string(const char *string, char outputBuffer[65]) {
    unsigned char hash[32];
    SHA256_CTX sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, (const unsigned char *)string, strlen(string));
    sha256_final(&sha256, hash);
    for(int i = 0; i < 32; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

/* ================================================================
   SESSION MANAGEMENT
   ================================================================ */



#define SESSION_COOKIE_NAME "vs_session"
#define SESSION_EXPIRE_HOURS 24
#define SESSION_ID_LEN 32

typedef struct {
    char session_id[SESSION_ID_LEN + 1];
    int  user_id;
    char user_name[128];
    char user_cnic[20];
    char user_email[120];
    int  is_admin;
    int  valid;
} Session;

void session_init(sqlite3 *db);
void session_generate_id(char *out, int len);
int  session_create(sqlite3 *db, int user_id, const char *name, const char *cnic, const char *email, int is_admin, char *out_session_id);
Session session_get(sqlite3 *db, const char *session_id);
void session_destroy(sqlite3 *db, const char *session_id);
void session_cleanup_expired(sqlite3 *db);


/* ================================================================
   SESSION IMPLEMENTATION
   ================================================================ */


void session_init(sqlite3 *db) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS sessions ("
        "session_id TEXT PRIMARY KEY,"
        "user_id INTEGER NOT NULL,"
        "user_name TEXT,"
        "user_cnic TEXT,"
        "user_email TEXT,"
        "is_admin INTEGER DEFAULT 0,"
        "expires_at DATETIME"
        ");";
    char *err = NULL;
    sqlite3_exec(db, sql, 0, 0, &err);
    if (err) sqlite3_free(err);
}

void session_generate_id(char *out, int len) {
    static const char chars[] = "abcdef0123456789";
    srand((unsigned int)time(NULL) ^ (unsigned int)(size_t)out);
    for (int i = 0; i < len; i++) {
        out[i] = chars[rand() % 16];
    }
    out[len] = '\0';
}

int session_create(sqlite3 *db, int user_id, const char *name, const char *cnic,
                   const char *email, int is_admin, char *out_session_id) {
    char sid[SESSION_ID_LEN + 1];
    session_generate_id(sid, SESSION_ID_LEN);

    const char *sql =
        "INSERT INTO sessions (session_id, user_id, user_name, user_cnic, user_email, is_admin, expires_at) "
        "VALUES (?, ?, ?, ?, ?, ?, datetime('now', '+24 hours'));";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, sid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, cnic, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, is_admin);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        strncpy(out_session_id, sid, SESSION_ID_LEN + 1);
        return 1;
    }
    return 0;
}

Session session_get(sqlite3 *db, const char *session_id) {
    Session s;
    memset(&s, 0, sizeof(s));
    s.valid = 0;

    if (!session_id || strlen(session_id) == 0) return s;

    const char *sql =
        "SELECT user_id, user_name, user_cnic, user_email, is_admin "
        "FROM sessions WHERE session_id = ? AND expires_at > datetime('now');";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return s;

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        s.user_id  = sqlite3_column_int(stmt, 0);
        const char *nm = (const char *)sqlite3_column_text(stmt, 1);
        const char *cn = (const char *)sqlite3_column_text(stmt, 2);
        const char *em = (const char *)sqlite3_column_text(stmt, 3);
        s.is_admin = sqlite3_column_int(stmt, 4);
        if (nm) strncpy(s.user_name,  nm, 127);
        if (cn) strncpy(s.user_cnic,  cn, 19);
        if (em) strncpy(s.user_email, em, 119);
        strncpy(s.session_id, session_id, SESSION_ID_LEN);
        s.valid = 1;
    }
    sqlite3_finalize(stmt);
    return s;
}

void session_destroy(sqlite3 *db, const char *session_id) {
    const char *sql = "DELETE FROM sessions WHERE session_id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void session_cleanup_expired(sqlite3 *db) {
    sqlite3_exec(db, "DELETE FROM sessions WHERE expires_at <= datetime('now');", 0, 0, NULL);
}

/* ================================================================
   DATABASE WRAPPER
   ================================================================ */



/* ---- Init ---- */
int db_init(const char *path);
sqlite3 *db_get(void);

/* ---- Users ---- */
int db_create_user(const char *cnic, const char *name, const char *email, const char *hash, int is_admin);
int db_verify_user_by_email(const char *email, const char *hash, int *uid, int *is_admin, int *is_deleted);
int db_verify_user_by_cnic(const char *cnic, const char *hash, int *uid, int *is_admin, int *is_deleted);
int db_get_user(int user_id, char *name, char *cnic, char *email, int *is_admin, int *is_deleted, char *created_at);
int db_update_user(int user_id, const char *name, const char *email);
int db_update_password(int user_id, const char *new_hash);
int db_ban_user(int user_id);
int db_unban_user(int user_id);
int db_user_is_banned(int user_id);

/* ---- Elections ---- */
int db_create_election(const char *title, const char *desc, const char *start, const char *end, const char *status, int *out_id);
int db_update_election(int eid, const char *title, const char *desc, const char *start, const char *end);
int db_delete_election(int eid, int by_uid);
int db_get_total_votes_in_election(int eid);
void db_update_election_statuses(void);

/* ---- Election Voters ---- */
int db_set_election_voters(int eid, int *voter_ids, int count);
int db_is_eligible(int eid, int uid);

/* ---- Candidates ---- */
int db_add_candidate(int user_id, const char *cnic, const char *name, const char *desc, int eid, const char *img, int app_id);
int db_delete_candidate(int candidate_id);

/* ---- Applications ---- */
int db_submit_application(int user_id, int eid, const char *desc, const char *img_path, int *out_id);
int db_approve_application(int app_id, int admin_uid);
int db_reject_application(int app_id, int admin_uid, const char *reason);
int db_user_has_applied(int user_id, int eid);
int db_user_is_candidate(int user_id, int eid);

/* ---- Votes ---- */
int db_cast_vote(int user_id, int candidate_id, int eid, const char *ip);
int db_user_has_voted(int user_id, int eid);

/* ---- Batch Ops ---- */
int db_clear_voters(void);
int db_clear_elections(void);
int db_reset_all(void);

/* ---- Password Reset Requests ---- */
int  db_create_password_reset_request(int user_id, const char *email);
int  db_has_pending_reset_request(int user_id);
int  db_resolve_password_reset(int req_id, const char *new_hash);
/* callback row: req_id, user_id, name, email, cnic, requested_at, status */
typedef void (*db_reset_row_cb)(int req_id, int uid, const char *name,
                                const char *email, const char *cnic,
                                const char *requested_at, const char *status,
                                void *user_data);
void db_foreach_reset_request(db_reset_row_cb cb, void *user_data);


/* ================================================================
   DATABASE IMPLEMENTATION
   ================================================================ */


static sqlite3 *g_db = NULL;

static const char *SCHEMA =
    "CREATE TABLE IF NOT EXISTS sessions ("
    "session_id TEXT PRIMARY KEY,"
    "user_id INTEGER NOT NULL,"
    "user_name TEXT,"
    "user_cnic TEXT,"
    "user_email TEXT,"
    "is_admin INTEGER DEFAULT 0,"
    "expires_at DATETIME);"

    "CREATE TABLE IF NOT EXISTS users ("
    "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "cnic TEXT UNIQUE NOT NULL,"
    "email TEXT UNIQUE NOT NULL,"
    "password_hash TEXT NOT NULL,"
    "is_admin INTEGER DEFAULT 0,"
    "is_deleted INTEGER DEFAULT 0,"
    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"

    "CREATE TABLE IF NOT EXISTS elections ("
    "election_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "title TEXT NOT NULL,"
    "description TEXT,"
    "status TEXT DEFAULT 'upcoming',"
    "start_date DATETIME NOT NULL,"
    "end_date DATETIME NOT NULL,"
    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "is_deleted INTEGER DEFAULT 0,"
    "deleted_at DATETIME,"
    "deleted_by INTEGER);"

    "CREATE TABLE IF NOT EXISTS election_voters ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "election_id INTEGER NOT NULL,"
    "user_id INTEGER NOT NULL,"
    "added_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "UNIQUE(election_id, user_id));"

    "CREATE TABLE IF NOT EXISTS candidates ("
    "candidate_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id INTEGER NOT NULL,"
    "cnic TEXT NOT NULL,"
    "name TEXT NOT NULL,"
    "description TEXT,"
    "election_id INTEGER NOT NULL,"
    "image_path TEXT DEFAULT 'default-candidate.png',"
    "application_id INTEGER,"
    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"

    "CREATE TABLE IF NOT EXISTS candidate_applications ("
    "application_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id INTEGER NOT NULL,"
    "election_id INTEGER NOT NULL,"
    "description TEXT,"
    "image_path TEXT DEFAULT 'default-candidate.png',"
    "status TEXT DEFAULT 'pending',"
    "applied_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "reviewed_at DATETIME,"
    "reviewed_by INTEGER,"
    "rejection_reason TEXT,"
    "UNIQUE(user_id, election_id));"

    "CREATE TABLE IF NOT EXISTS votes ("
    "vote_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id INTEGER NOT NULL,"
    "candidate_id INTEGER NOT NULL,"
    "election_id INTEGER NOT NULL,"
    "voted_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "ip_address TEXT,"
    "UNIQUE(user_id, election_id));"

    "CREATE TABLE IF NOT EXISTS password_reset_requests ("
    "req_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id INTEGER NOT NULL,"
    "email TEXT NOT NULL,"
    "status TEXT DEFAULT 'pending',"
    "requested_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "resolved_at DATETIME,"
    "UNIQUE(user_id, status));";

int db_init(const char *path) {
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(g_db));
        return 0;
    }
    sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", 0, 0, NULL);
    char *err = NULL;
    /* Execute schema one statement at a time */
    const char *ptr = SCHEMA;
    while (*ptr) {
        sqlite3_stmt *tmp;
        const char *tail;
        int rc = sqlite3_prepare_v2(g_db, ptr, -1, &tmp, &tail);
        if (rc == SQLITE_OK) {
            sqlite3_step(tmp);
            sqlite3_finalize(tmp);
        }
        if (tail == ptr) break;
        ptr = tail;
    }
    (void)err;
    return 1;
}

sqlite3 *db_get(void) { return g_db; }

/* ================================================================
   USERS
   ================================================================ */

int db_create_user(const char *cnic, const char *name, const char *email,
                   const char *hash, int is_admin) {
    const char *sql =
        "INSERT OR IGNORE INTO users (cnic, name, email, password_hash, is_admin) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, cnic,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, hash,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, is_admin);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

static int verify_user(const char *field, const char *val, const char *hash,
                        int *uid, int *is_admin, int *is_deleted) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT user_id, is_admin, is_deleted FROM users "
        "WHERE %s = ? AND password_hash = ?;", field);
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, val,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_TRANSIENT);
    int ok = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *uid        = sqlite3_column_int(stmt, 0);
        *is_admin   = sqlite3_column_int(stmt, 1);
        *is_deleted = sqlite3_column_int(stmt, 2);
        ok = 1;
    }
    sqlite3_finalize(stmt);
    return ok;
}

int db_verify_user_by_email(const char *email, const char *hash,
                             int *uid, int *is_admin, int *is_deleted) {
    return verify_user("email", email, hash, uid, is_admin, is_deleted);
}

int db_verify_user_by_cnic(const char *cnic, const char *hash,
                            int *uid, int *is_admin, int *is_deleted) {
    return verify_user("cnic", cnic, hash, uid, is_admin, is_deleted);
}

int db_get_user(int user_id, char *name, char *cnic, char *email,
                int *is_admin, int *is_deleted, char *created_at) {
    const char *sql =
        "SELECT name, cnic, email, is_admin, is_deleted, created_at "
        "FROM users WHERE user_id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    int ok = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *n = (const char *)sqlite3_column_text(stmt, 0);
        const char *c = (const char *)sqlite3_column_text(stmt, 1);
        const char *e = (const char *)sqlite3_column_text(stmt, 2);
        const char *dt= (const char *)sqlite3_column_text(stmt, 5);
        if (name)       { if(n) strncpy(name, n, 127); else name[0]=0; }
        if (cnic)       { if(c) strncpy(cnic, c, 19);  else cnic[0]=0; }
        if (email)      { if(e) strncpy(email,e, 119);  else email[0]=0; }
        if (is_admin)   *is_admin   = sqlite3_column_int(stmt, 3);
        if (is_deleted) *is_deleted = sqlite3_column_int(stmt, 4);
        if (created_at) { if(dt) strncpy(created_at, dt, 31); else created_at[0]=0; }
        ok = 1;
    }
    sqlite3_finalize(stmt);
    return ok;
}

int db_update_user(int user_id, const char *name, const char *email) {
    const char *sql = "UPDATE users SET name = ?, email = ? WHERE user_id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, name,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_update_password(int user_id, const char *new_hash) {
    const char *sql = "UPDATE users SET password_hash = ? WHERE user_id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, new_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_ban_user(int user_id) {
    const char *sql = "UPDATE users SET is_deleted = 1 WHERE user_id = ? AND is_admin = 0;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

int db_unban_user(int user_id) {
    const char *sql = "UPDATE users SET is_deleted = 0 WHERE user_id = ? AND is_admin = 0;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

int db_user_is_banned(int user_id) {
    const char *sql = "SELECT is_deleted FROM users WHERE user_id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    int banned = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) banned = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return banned;
}

/* ================================================================
   ELECTIONS
   ================================================================ */

int db_create_election(const char *title, const char *desc,
                       const char *start, const char *end,
                       const char *status, int *out_id) {
    const char *sql =
        "INSERT INTO elections (title, description, start_date, end_date, status) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, title,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, desc,   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, start,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, end,    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, status, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
        if (out_id) *out_id = (int)sqlite3_last_insert_rowid(g_db);
        return 1;
    }
    return 0;
}

int db_update_election(int eid, const char *title, const char *desc,
                       const char *start, const char *end) {
    const char *sql =
        "UPDATE elections SET title=?, description=?, start_date=?, end_date=? "
        "WHERE election_id=? AND is_deleted=0;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, desc,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, start, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, end,   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, eid);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_delete_election(int eid, int by_uid) {
    const char *sql =
        "UPDATE elections SET is_deleted=1, deleted_at=datetime('now'), deleted_by=? "
        "WHERE election_id=? AND status='upcoming' AND "
        "(SELECT COUNT(*) FROM votes WHERE election_id=?)=0;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, by_uid);
    sqlite3_bind_int(stmt, 2, eid);
    sqlite3_bind_int(stmt, 3, eid);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

int db_get_total_votes_in_election(int eid) {
    const char *sql = "SELECT COUNT(*) FROM votes WHERE election_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, eid);
    int cnt = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) cnt = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return cnt;
}

void db_update_election_statuses(void) {
    sqlite3_exec(g_db,
        "UPDATE elections SET status='active' "
        "WHERE is_deleted=0 AND status!='closed' "
        "AND datetime('now') >= start_date AND datetime('now') < end_date;",
        0, 0, NULL);
    sqlite3_exec(g_db,
        "UPDATE elections SET status='closed' "
        "WHERE is_deleted=0 AND datetime('now') >= end_date;",
        0, 0, NULL);
    sqlite3_exec(g_db,
        "UPDATE elections SET status='upcoming' "
        "WHERE is_deleted=0 AND status!='active' AND status!='closed' "
        "AND datetime('now') < start_date;",
        0, 0, NULL);
}

/* ================================================================
   ELECTION VOTERS
   ================================================================ */

int db_set_election_voters(int eid, int *voter_ids, int count) {
    sqlite3_exec(g_db, "BEGIN;", 0, 0, NULL);
    char del[128];
    snprintf(del, sizeof(del),
        "DELETE FROM election_voters WHERE election_id=%d;", eid);
    sqlite3_exec(g_db, del, 0, 0, NULL);

    const char *ins =
        "INSERT OR IGNORE INTO election_voters (election_id, user_id) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, ins, -1, &stmt, NULL);
    for (int i = 0; i < count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, eid);
        sqlite3_bind_int(stmt, 2, voter_ids[i]);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(g_db, "COMMIT;", 0, 0, NULL);
    return 1;
}

int db_is_eligible(int eid, int uid) {
    const char *sql =
        "SELECT 1 FROM election_voters WHERE election_id=? AND user_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, eid);
    sqlite3_bind_int(stmt, 2, uid);
    int ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

/* ================================================================
   CANDIDATES
   ================================================================ */

int db_add_candidate(int user_id, const char *cnic, const char *name,
                     const char *desc, int eid, const char *img, int app_id) {
    const char *sql =
        "INSERT INTO candidates (user_id, cnic, name, description, election_id, image_path, application_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int (stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, cnic,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, desc ? desc : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, eid);
    sqlite3_bind_text(stmt, 6, img  ? img  : "default-candidate.png", -1, SQLITE_TRANSIENT);
    if (app_id > 0) sqlite3_bind_int(stmt, 7, app_id);
    else            sqlite3_bind_null(stmt, 7);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_delete_candidate(int candidate_id) {
    const char *sql =
        "DELETE FROM candidates WHERE candidate_id=? AND "
        "(SELECT status FROM elections WHERE election_id="
        "(SELECT election_id FROM candidates WHERE candidate_id=?))='upcoming';";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, candidate_id);
    sqlite3_bind_int(stmt, 2, candidate_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

int db_user_is_candidate(int user_id, int eid) {
    const char *sql =
        "SELECT 1 FROM candidates WHERE user_id=? AND election_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, eid);
    int ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

/* ================================================================
   APPLICATIONS
   ================================================================ */

int db_submit_application(int user_id, int eid, const char *desc,
                          const char *img_path, int *out_id) {
    const char *sql =
        "INSERT INTO candidate_applications (user_id, election_id, description, image_path) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int (stmt, 1, user_id);
    sqlite3_bind_int (stmt, 2, eid);
    sqlite3_bind_text(stmt, 3, desc     ? desc     : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, img_path ? img_path : "default-candidate.png", -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE) {
        if (out_id) *out_id = (int)sqlite3_last_insert_rowid(g_db);
        return 1;
    }
    return 0;
}

int db_approve_application(int app_id, int admin_uid) {
    /* Read application details */
    const char *sel =
        "SELECT a.user_id, a.election_id, a.description, a.image_path, "
        "       u.cnic, u.name "
        "FROM candidate_applications a "
        "JOIN users u ON u.user_id = a.user_id "
        "WHERE a.application_id = ? AND a.status = 'pending';";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sel, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, app_id);

    int uid = 0, eid = 0;
    char desc[1024]="", img[256]="", cnic[20]="", name[128]="";

    if (sqlite3_step(stmt) != SQLITE_ROW) { sqlite3_finalize(stmt); return 0; }
    uid = sqlite3_column_int(stmt, 0);
    eid = sqlite3_column_int(stmt, 1);
    const char *d = (const char*)sqlite3_column_text(stmt, 2);
    const char *i = (const char*)sqlite3_column_text(stmt, 3);
    const char *c = (const char*)sqlite3_column_text(stmt, 4);
    const char *n = (const char*)sqlite3_column_text(stmt, 5);
    if (d) strncpy(desc, d, 1023);
    if (i) strncpy(img,  i, 255);
    if (c) strncpy(cnic, c, 19);
    if (n) strncpy(name, n, 127);
    sqlite3_finalize(stmt);

    /* Create candidate */
    if (!db_add_candidate(uid, cnic, name, desc, eid, img, app_id)) return 0;

    /* Update application status */
    const char *upd =
        "UPDATE candidate_applications SET status='approved', "
        "reviewed_at=datetime('now'), reviewed_by=? WHERE application_id=?;";
    if (sqlite3_prepare_v2(g_db, upd, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, admin_uid);
    sqlite3_bind_int(stmt, 2, app_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_reject_application(int app_id, int admin_uid, const char *reason) {
    const char *sql =
        "UPDATE candidate_applications SET status='rejected', "
        "reviewed_at=datetime('now'), reviewed_by=?, rejection_reason=? "
        "WHERE application_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int (stmt, 1, admin_uid);
    sqlite3_bind_text(stmt, 2, reason ? reason : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, app_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_user_has_applied(int user_id, int eid) {
    const char *sql =
        "SELECT application_id FROM candidate_applications "
        "WHERE user_id=? AND election_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, eid);
    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return id;
}

/* ================================================================
   VOTES
   ================================================================ */

int db_cast_vote(int user_id, int candidate_id, int eid, const char *ip) {
    const char *sql =
        "INSERT OR IGNORE INTO votes (user_id, candidate_id, election_id, ip_address) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int (stmt, 1, user_id);
    sqlite3_bind_int (stmt, 2, candidate_id);
    sqlite3_bind_int (stmt, 3, eid);
    sqlite3_bind_text(stmt, 4, ip ? ip : "", -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE && sqlite3_changes(g_db) > 0);
}

int db_user_has_voted(int user_id, int eid) {
    const char *sql =
        "SELECT 1 FROM votes WHERE user_id=? AND election_id=?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, eid);
    int ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

/* ================================================================
   BATCH OPS
   ================================================================ */

int db_clear_voters(void) {
    sqlite3_exec(g_db, "DELETE FROM votes;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidate_applications;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidates;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM election_voters;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM users WHERE is_admin=0;", 0, 0, NULL);
    return 1;
}

int db_clear_elections(void) {
    sqlite3_exec(g_db, "DELETE FROM votes;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidate_applications;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidates;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM election_voters;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM elections;", 0, 0, NULL);
    return 1;
}

int db_reset_all(void) {
    sqlite3_exec(g_db, "DELETE FROM sessions;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM votes;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidate_applications;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM candidates;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM election_voters;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM elections;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM users;", 0, 0, NULL);
    sqlite3_exec(g_db, "DELETE FROM password_reset_requests;", 0, 0, NULL);
    return 1;
}

/* ================================================================
   PASSWORD RESET REQUESTS
   ================================================================ */

int db_create_password_reset_request(int user_id, const char *email) {
    /* Remove any existing pending request for this user first */
    const char *del = "DELETE FROM password_reset_requests WHERE user_id=? AND status='pending';";
    sqlite3_stmt *d;
    if (sqlite3_prepare_v2(g_db, del, -1, &d, NULL) == SQLITE_OK) {
        sqlite3_bind_int(d, 1, user_id); sqlite3_step(d); sqlite3_finalize(d);
    }
    const char *sql =
        "INSERT INTO password_reset_requests (user_id, email) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int (stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_has_pending_reset_request(int user_id) {
    const char *sql =
        "SELECT 1 FROM password_reset_requests WHERE user_id=? AND status='pending';";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, user_id);
    int ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

int db_resolve_password_reset(int req_id, const char *new_hash) {
    /* Get user_id from request */
    const char *sel = "SELECT user_id FROM password_reset_requests WHERE req_id=?;";
    sqlite3_stmt *s;
    if (sqlite3_prepare_v2(g_db, sel, -1, &s, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(s, 1, req_id);
    int uid = 0;
    if (sqlite3_step(s) == SQLITE_ROW) uid = sqlite3_column_int(s, 0);
    sqlite3_finalize(s);
    if (!uid) return 0;
    /* Update password */
    db_update_password(uid, new_hash);
    /* Mark request resolved */
    const char *upd =
        "UPDATE password_reset_requests SET status='resolved', "
        "resolved_at=datetime('now') WHERE req_id=?;";
    sqlite3_stmt *u;
    if (sqlite3_prepare_v2(g_db, upd, -1, &u, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(u, 1, req_id);
    int rc = sqlite3_step(u);
    sqlite3_finalize(u);
    return rc == SQLITE_DONE;
}

void db_foreach_reset_request(db_reset_row_cb cb, void *user_data) {
    const char *sql =
        "SELECT r.req_id, r.user_id, u.name, r.email, u.cnic, r.requested_at, r.status "
        "FROM password_reset_requests r "
        "JOIN users u ON u.user_id = r.user_id "
        "ORDER BY r.status ASC, r.requested_at DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int req_id = sqlite3_column_int(stmt, 0);
        int uid    = sqlite3_column_int(stmt, 1);
        const char *nm = (const char*)sqlite3_column_text(stmt, 2);
        const char *em = (const char*)sqlite3_column_text(stmt, 3);
        const char *cn = (const char*)sqlite3_column_text(stmt, 4);
        const char *ra = (const char*)sqlite3_column_text(stmt, 5);
        const char *st = (const char*)sqlite3_column_text(stmt, 6);
        cb(req_id, uid, nm?nm:"", em?em:"", cn?cn:"", ra?ra:"", st?st:"", user_data);
    }
    sqlite3_finalize(stmt);
}

/* ================================================================
   UTILITIES
   ================================================================ */


void get_iso_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", t);
}

/* ================================================================
   EMBEDDED HTML TEMPLATES
   ================================================================ */

static const char *TPL_ADMIN_ADD_VOTER =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Add Voter - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/voters\">← Manage Voters</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:700px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-6\">Add New Voter</h1>\n"
    "    <form method=\"POST\" action=\"/admin/voters/add\">\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"name\" class=\"form-label\">Full Name</label>\n"
    "        <input type=\"text\" id=\"name\" name=\"name\" class=\"form-control\" placeholder=\"e.g. Ali Hassan\" value=\"{{NAME_VAL}}\" required>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"cnic\" class=\"form-label\">CNIC Number</label>\n"
    "        <input type=\"text\" id=\"cnic\" name=\"cnic\" class=\"form-control\" placeholder=\"0000000000000 (13 digits)\" value=\"{{CNIC_VAL}}\" required maxlength=\"15\">\n"
    "        <span class=\"form-text\">Enter 13 digit CNIC without dashes</span>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"email\" class=\"form-label\">Email Address</label>\n"
    "        <input type=\"email\" id=\"email\" name=\"email\" class=\"form-control\" placeholder=\"voter@example.com\" value=\"{{EMAIL_VAL}}\" required>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"password\" class=\"form-label\">Initial Password</label>\n"
    "        <input type=\"password\" id=\"password\" name=\"password\" class=\"form-control\" placeholder=\"Minimum 8 characters\" required minlength=\"8\">\n"
    "        <span class=\"form-text\">The voter will use this password to login with their CNIC</span>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary btn-lg\">Add Voter</button>\n"
    "        <a href=\"/admin/voters\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_APPLICATIONS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>All Applications - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/dashboard\">Dashboard</a></li>\n"
    "        <li><a href=\"/admin/elections\">Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <h1 class=\"mb-4\">All Candidate Applications</h1>\n"
    "  <!-- Filter tabs -->\n"
    "  <div class=\"d-flex gap-2 mb-4\">\n"
    "    <a href=\"/admin/applications\" class=\"btn btn-sm {{STATUS_FILTER}}==all?btn-primary:btn-outline\">All</a>\n"
    "    <a href=\"/admin/applications?status=pending\" class=\"btn btn-sm btn-warning\">Pending</a>\n"
    "    <a href=\"/admin/applications?status=approved\" class=\"btn btn-sm btn-success\">Approved</a>\n"
    "    <a href=\"/admin/applications?status=rejected\" class=\"btn btn-sm btn-danger\">Rejected</a>\n"
    "  </div>\n"
    "  <div class=\"card-glass p-6\">\n"
    "    {{NO_APPS_MSG}}\n"
    "    <div class=\"table-responsive\">\n"
    "      <table class=\"table\">\n"
    "        <thead><tr><th>Applicant</th><th>CNIC</th><th>Election</th><th>Applied At</th><th>Status</th></tr></thead>\n"
    "        <tbody>{{APPLICATION_ROWS}}</tbody>\n"
    "      </table>\n"
    "    </div>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_CHANGE_PASSWORD =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Change Password - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/dashboard\">← Dashboard</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:600px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-2\">Change Password</h1>\n"
    "    <p class=\"text-gray mb-6\">Admin: <strong>{{USER_NAME}}</strong></p>\n"
    "    <form method=\"POST\" action=\"/admin/change-password\">\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"current_password\" class=\"form-label\">Current Password</label>\n"
    "        <input type=\"password\" id=\"current_password\" name=\"current_password\" class=\"form-control\" required>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"new_password\" class=\"form-label\">New Password</label>\n"
    "        <input type=\"password\" id=\"new_password\" name=\"new_password\" class=\"form-control\"\n"
    "            placeholder=\"Minimum 8 characters\" required minlength=\"8\" oninput=\"checkStrength(this.value)\">\n"
    "        <div class=\"progress mt-2\" style=\"height:8px;\">\n"
    "          <div id=\"strength-bar\" class=\"progress-bar\" style=\"width:0%;transition:width 0.3s,background 0.3s;\"></div>\n"
    "        </div>\n"
    "        <span id=\"strength-label\" class=\"form-text\"></span>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"confirm_password\" class=\"form-label\">Confirm New Password</label>\n"
    "        <input type=\"password\" id=\"confirm_password\" name=\"confirm_password\" class=\"form-control\" required>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary btn-lg\">🔒 Change Password</button>\n"
    "        <a href=\"/admin/dashboard\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "function checkStrength(pw){\n"
    "  let s=0;\n"
    "  if(pw.length>=8)s++;if(/[A-Z]/.test(pw))s++;if(/[0-9]/.test(pw))s++;if(/[^A-Za-z0-9]/.test(pw))s++;\n"
    "  const bar=document.getElementById('strength-bar');\n"
    "  const lbl=document.getElementById('strength-label');\n"
    "  const cols=['#ef4444','#f59e0b','#eab308','#22c55e'];\n"
    "  const lbls=['Weak','Fair','Good','Strong'];\n"
    "  bar.style.width=(s*25)+'%';\n"
    "  bar.style.background=cols[s-1]||'#ef4444';\n"
    "  lbl.textContent=s>0?lbls[s-1]:'';\n"
    "}\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_CREATE_ELECTION =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Create Election - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/elections\">← Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:900px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-6\">Create New Election</h1>\n"
    "    <form method=\"POST\" action=\"/admin/elections/create\">\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"title\" class=\"form-label\">Election Title</label>\n"
    "        <input type=\"text\" id=\"title\" name=\"title\" class=\"form-control\" placeholder=\"e.g. Student Council Election 2026\" required>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"description\" class=\"form-label\">Description</label>\n"
    "        <textarea id=\"description\" name=\"description\" class=\"form-control\" rows=\"3\" placeholder=\"Describe this election...\"></textarea>\n"
    "      </div>\n"
    "      <div class=\"row g-3\">\n"
    "        <div class=\"col-md-6\">\n"
    "          <div class=\"form-group\">\n"
    "            <label for=\"start_date\" class=\"form-label\">Start Date &amp; Time</label>\n"
    "            <input type=\"datetime-local\" id=\"start_date\" name=\"start_date\" class=\"form-control\" required>\n"
    "          </div>\n"
    "        </div>\n"
    "        <div class=\"col-md-6\">\n"
    "          <div class=\"form-group\">\n"
    "            <label for=\"end_date\" class=\"form-label\">End Date &amp; Time</label>\n"
    "            <input type=\"datetime-local\" id=\"end_date\" name=\"end_date\" class=\"form-control\" required>\n"
    "          </div>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label class=\"form-label\">Select Eligible Voters</label>\n"
    "        <p class=\"text-sm text-gray mb-3\">Only selected voters will be able to vote and apply as candidates</p>\n"
    "        <div class=\"card-glass p-4\" style=\"max-height:400px;overflow-y:auto;\">\n"
    "          <div class=\"mb-3\">\n"
    "            <button type=\"button\" class=\"btn btn-sm btn-secondary\" onclick=\"document.querySelectorAll('.voter-checkbox').forEach(c=>c.checked=true)\">Select All</button>\n"
    "            <button type=\"button\" class=\"btn btn-sm btn-secondary\" onclick=\"document.querySelectorAll('.voter-checkbox').forEach(c=>c.checked=false)\">Deselect All</button>\n"
    "          </div>\n"
    "          <div class=\"row g-2\">\n"
    "            {{VOTER_CHECKBOXES}}\n"
    "          </div>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary btn-lg\">Create Election</button>\n"
    "        <a href=\"/admin/elections\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_DASHBOARD =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Admin Dashboard - Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><span style=\"color:var(--gray-600);\">Welcome, <strong>{{USER_NAME}}</strong></span></li>\n"
    "        <li><a href=\"/admin/dashboard\">Dashboard</a></li>\n"
    "        <li><a href=\"/admin/elections\">Elections</a></li>\n"
    "        <li><a href=\"/admin/voters\">Voters</a></li>\n"
    "        <li><a href=\"/admin/change-password\">🔒 Change Password</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"mb-8\">\n"
    "    <h1>Admin Dashboard</h1>\n"
    "    <p class=\"lead text-gray\">Manage elections, voters, and applications</p>\n"
    "  </div>\n"
    "\n"
    "  <!-- Stats -->\n"
    "  <div class=\"row g-4 mb-6\">\n"
    "    <div class=\"col-md-3\">\n"
    "      <a href=\"/admin/elections\" style=\"text-decoration:none;\">\n"
    "      <div class=\"card-glass p-4\" style=\"transition:transform 0.2s;\" onmouseover=\"this.style.transform='translateY(-3px)'\" onmouseout=\"this.style.transform=''\">\n"
    "        <h3 class=\"h1 mb-2\">{{TOTAL_ELECTIONS}}</h3>\n"
    "        <p class=\"text-gray mb-0\">Total Elections</p>\n"
    "      </div></a>\n"
    "    </div>\n"
    "    <div class=\"col-md-3\">\n"
    "      <a href=\"/admin/elections\" style=\"text-decoration:none;\">\n"
    "      <div class=\"card-glass p-4\" style=\"transition:transform 0.2s;\" onmouseover=\"this.style.transform='translateY(-3px)'\" onmouseout=\"this.style.transform=''\">\n"
    "        <h3 class=\"h1 mb-2 text-success\">{{ACTIVE_ELECTIONS}}</h3>\n"
    "        <p class=\"text-gray mb-0\">Active Elections</p>\n"
    "      </div></a>\n"
    "    </div>\n"
    "    <div class=\"col-md-3\">\n"
    "      <a href=\"/admin/voters\" style=\"text-decoration:none;\">\n"
    "      <div class=\"card-glass p-4\" style=\"transition:transform 0.2s;\" onmouseover=\"this.style.transform='translateY(-3px)'\" onmouseout=\"this.style.transform=''\">\n"
    "        <h3 class=\"h1 mb-2\">{{TOTAL_VOTERS}}</h3>\n"
    "        <p class=\"text-gray mb-0\">Registered Voters</p>\n"
    "      </div></a>\n"
    "    </div>\n"
    "    <div class=\"col-md-3\">\n"
    "      <a href=\"/admin/voters?filter=resets\" style=\"text-decoration:none;\">\n"
    "      <div class=\"card-glass p-4\" style=\"transition:transform 0.2s;border:{{RESET_BORDER}};\" onmouseover=\"this.style.transform='translateY(-3px)'\" onmouseout=\"this.style.transform=''\">\n"
    "        <h3 class=\"h1 mb-2\" style=\"color:{{RESET_COLOR}}\">{{RESET_REQUESTS}}</h3>\n"
    "        <p class=\"text-gray mb-0\">🔑 Reset Requests</p>\n"
    "      </div></a>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Quick Actions -->\n"
    "  <div class=\"card-glass p-6 mb-6\">\n"
    "    <h2 class=\"h3 mb-4\">Quick Actions</h2>\n"
    "    <div class=\"d-flex gap-3 flex-wrap\">\n"
    "      <a href=\"/admin/elections/create\" class=\"btn btn-primary\">➕ Create Election</a>\n"
    "      <a href=\"/admin/voters/add\" class=\"btn btn-secondary\">👥 Add Voter</a>\n"
    "      <a href=\"/admin/applications\" class=\"btn btn-warning\">📝 Applications ({{PENDING_APPS_STR}})</a>\n"
    "      <a href=\"/admin/voters?filter=resets\" class=\"btn\" style=\"background:#dc3545;color:#fff;\">🔑 Reset Requests ({{RESET_REQUESTS}})</a>\n"
    "      <a href=\"/admin/voters\" class=\"btn btn-info\">🔍 All Voters</a>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Recent Elections -->\n"
    "  <div class=\"card-glass p-6 mb-6\">\n"
    "    <h2 class=\"h3 mb-4\">Recent Elections</h2>\n"
    "    <div class=\"table-responsive\">\n"
    "      <table class=\"table\">\n"
    "        <thead>\n"
    "          <tr>\n"
    "            <th>Title</th><th>Status</th><th>Start Date</th><th>Eligible Voters</th><th>Actions</th>\n"
    "          </tr>\n"
    "        </thead>\n"
    "        <tbody>{{RECENT_ROWS}}</tbody>\n"
    "      </table>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Pending Applications section (rendered conditionally by C) -->\n"
    "  {{PENDING_SECTION}}\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\">\n"
    "    <p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p>\n"
    "    <p class=\"text-sm\" style=\"margin-top:var(--space-2);\">BS Computer Science | Ziauddin University</p>\n"
    "  </div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "function rejectApplication(id){\n"
    "  const reason=prompt('Enter rejection reason:');\n"
    "  if(reason){\n"
    "    const form=document.createElement('form');\n"
    "    form.method='POST';\n"
    "    form.action='/admin/applications/'+id+'/reject';\n"
    "    const ri=document.createElement('input');ri.type='hidden';ri.name='reason';ri.value=reason;\n"
    "    form.appendChild(ri);document.body.appendChild(form);form.submit();\n"
    "  }\n"
    "}\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_EDIT_ELECTION =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Edit Election - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/elections\">← Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:800px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-6\">Edit Election</h1>\n"
    "    <form method=\"POST\" action=\"/admin/elections/{{ELECTION_ID}}/edit\">\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"title\" class=\"form-label\">Election Title</label>\n"
    "        <input type=\"text\" id=\"title\" name=\"title\" class=\"form-control\" value=\"{{ELECTION_TITLE}}\" required>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"description\" class=\"form-label\">Description</label>\n"
    "        <textarea id=\"description\" name=\"description\" class=\"form-control\" rows=\"3\">{{ELECTION_DESC}}</textarea>\n"
    "      </div>\n"
    "      <div class=\"row g-3\">\n"
    "        <div class=\"col-md-6\">\n"
    "          <div class=\"form-group\">\n"
    "            <label for=\"start_date\" class=\"form-label\">Start Date &amp; Time</label>\n"
    "            <input type=\"datetime-local\" id=\"start_date\" name=\"start_date\" class=\"form-control\" value=\"{{START_DATE}}\" required>\n"
    "          </div>\n"
    "        </div>\n"
    "        <div class=\"col-md-6\">\n"
    "          <div class=\"form-group\">\n"
    "            <label for=\"end_date\" class=\"form-label\">End Date &amp; Time</label>\n"
    "            <input type=\"datetime-local\" id=\"end_date\" name=\"end_date\" class=\"form-control\" value=\"{{END_DATE}}\" required>\n"
    "          </div>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary\">Save Changes</button>\n"
    "        <a href=\"/admin/elections\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_EDIT_VOTER =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Edit Voter - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/voters\">← Manage Voters</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\" style=\"max-width:740px;\">\n"
    "\n"
    "  <!-- EDIT FORM -->\n"
    "  <div class=\"card-glass p-8 mb-6\">\n"
    "    <h1 class=\"h2 mb-1\">✏️ Edit Voter</h1>\n"
    "    <p class=\"text-gray mb-6\">Update voter name or reset their password. CNIC and email are permanent identifiers and cannot be changed.</p>\n"
    "    <form method=\"POST\" action=\"/admin/voters/{{VOTER_ID}}/edit\">\n"
    "      <div class=\"row g-4 mb-4\">\n"
    "        <div class=\"col-md-6\">\n"
    "          <label class=\"form-label\" style=\"font-weight:600;\">CNIC <span style=\"color:var(--gray-500);font-weight:400;\">(read-only)</span></label>\n"
    "          <input type=\"text\" class=\"form-control\" value=\"{{VOTER_CNIC}}\" disabled\n"
    "            style=\"background:var(--gray-100);cursor:not-allowed;color:var(--gray-600);\">\n"
    "        </div>\n"
    "        <div class=\"col-md-6\">\n"
    "          <label for=\"email\" class=\"form-label\" style=\"font-weight:600;\">Email Address</label>\n"
    "          <input type=\"email\" id=\"email\" name=\"email\" class=\"form-control\"\n"
    "            value=\"{{VOTER_EMAIL}}\" required placeholder=\"voter@email.com\">\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"name\" class=\"form-label\">Full Name</label>\n"
    "        <input type=\"text\" id=\"name\" name=\"name\" class=\"form-control\"\n"
    "          value=\"{{VOTER_NAME}}\" required minlength=\"2\" placeholder=\"Full legal name\">\n"
    "      </div>\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"new_password\" class=\"form-label\">New Password\n"
    "          <span style=\"color:var(--gray-500);font-weight:400;\">(leave blank to keep current)</span>\n"
    "        </label>\n"
    "        <input type=\"password\" id=\"new_password\" name=\"new_password\" class=\"form-control\"\n"
    "          placeholder=\"Min 6 characters\" minlength=\"6\">\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3 mt-2\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary\" id=\"saveVoterBtn\">💾 Save Changes</button>\n"
    "        <a href=\"/admin/voters\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "\n"
    "  <!-- DANGER ZONE: PERMANENT DELETE -->\n"
    "  <div class=\"card-glass p-6\" style=\"border:1px solid rgba(239,68,68,0.35);background:rgba(239,68,68,0.05);\">\n"
    "    <h2 class=\"h4 mb-2\" style=\"color:var(--danger-600);\">⚠️ Danger Zone</h2>\n"
    "    <p class=\"text-gray mb-4\">\n"
    "      Permanently delete this voter and all their associated data (votes, applications, candidacies).\n"
    "      <strong style=\"color:var(--danger-600);\">This action cannot be undone.</strong>\n"
    "      Use this only when the voter was registered with incorrect CNIC or email.\n"
    "    </p>\n"
    "    <form method=\"POST\" action=\"/admin/voters/{{VOTER_ID}}/delete\"\n"
    "      onsubmit=\"return confirm('⚠️ PERMANENT DELETE\\\\n\\\\nThis will completely remove voter {{VOTER_NAME}} and ALL their data from the database.\\\\n\\\\nThis CANNOT be undone!\\\\n\\\\nAre you absolutely sure?');\">\n"
    "      <button type=\"submit\" class=\"btn\" id=\"deleteVoterBtn\"\n"
    "        style=\"background:var(--danger-600);color:#fff;border:none;\">\n"
    "        🗑️ Permanently Delete Voter\n"
    "      </button>\n"
    "    </form>\n"
    "  </div>\n"
    "\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_ELECTION_APPLICATIONS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Applications - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/elections\">← Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <h1 class=\"mb-2\">Candidate Applications</h1>\n"
    "  <p class=\"text-gray mb-6\">Election: <strong>{{ELECTION_TITLE}}</strong></p>\n"
    "  <div class=\"card-glass p-6\">\n"
    "    {{APPLICATION_CARDS}}\n"
    "  </div>\n"
    "  <div class=\"mt-4\">\n"
    "    <a href=\"/admin/elections/{{ELECTION_ID}}/candidates\" class=\"btn btn-info\">Manage Candidates</a>\n"
    "    <a href=\"/admin/elections\" class=\"btn btn-outline ml-3\">← Back to Elections</a>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "function rejectApp(id){\n"
    "  const reason=prompt('Rejection reason:');\n"
    "  if(reason){\n"
    "    const form=document.createElement('form');\n"
    "    form.method='POST';\n"
    "    form.action='/admin/applications/'+id+'/reject';\n"
    "    const r=document.createElement('input');r.type='hidden';r.name='reason';r.value=reason;\n"
    "    form.appendChild(r);document.body.appendChild(form);form.submit();\n"
    "  }\n"
    "}\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_ELECTIONS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Manage Elections - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><span style=\"color:var(--gray-600);\"><strong>{{USER_NAME}}</strong></span></li>\n"
    "        <li><a href=\"/admin/dashboard\">Dashboard</a></li>\n"
    "        <li><a href=\"/admin/elections\">Elections</a></li>\n"
    "        <li><a href=\"/admin/voters\">Voters</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"d-flex justify-content-between align-items-center mb-6\">\n"
    "    <h1>Manage Elections</h1>\n"
    "    <a href=\"/admin/elections/create\" class=\"btn btn-primary\">➕ Create New Election</a>\n"
    "  </div>\n"
    "  <div class=\"card-glass p-6\">\n"
    "    {{NO_ELECTIONS_MSG}}\n"
    "    <div class=\"table-responsive\">\n"
    "      <table class=\"table\">\n"
    "        <thead>\n"
    "          <tr>\n"
    "            <th>Title</th><th>Status</th><th>Date Range</th><th>Voters</th><th>Candidates</th><th>Votes</th><th>Actions</th>\n"
    "          </tr>\n"
    "        </thead>\n"
    "        <tbody>{{ELECTION_ROWS}}</tbody>\n"
    "      </table>\n"
    "    </div>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_MANAGE_CANDIDATES =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Manage Candidates - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/elections\">← Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <h1 class=\"mb-2\">Manage Candidates</h1>\n"
    "  <p class=\"text-gray mb-6\">Election: <strong>{{ELECTION_TITLE}}</strong></p>\n"
    "\n"
    "  <!-- Current candidates -->\n"
    "  <div class=\"card-glass p-6 mb-6\">\n"
    "    <h2 class=\"h3 mb-4\">Current Candidates</h2>\n"
    "    {{NO_CAND_MSG}}\n"
    "    <div class=\"table-responsive\">\n"
    "      <table class=\"table\">\n"
    "        <thead><tr><th>Name</th><th>CNIC</th><th>Description</th><th>Actions</th></tr></thead>\n"
    "        <tbody>{{CANDIDATE_ROWS}}</tbody>\n"
    "      </table>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Add candidate form (only for upcoming elections) -->\n"
    "  {{ADD_FORM}}\n"
    "\n"
    "  <div class=\"mt-4\">\n"
    "    <a href=\"/admin/elections/{{ELECTION_ID}}/applications\" class=\"btn btn-secondary\">View Applications</a>\n"
    "    <a href=\"/admin/elections\" class=\"btn btn-outline\">← Back to Elections</a>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_MANAGE_ELECTION_VOTERS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Manage Election Voters - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/elections\">← Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:900px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-2\">Manage Eligible Voters</h1>\n"
    "    <p class=\"text-gray mb-6\">Election: <strong>{{ELECTION_TITLE}}</strong></p>\n"
    "    <form method=\"POST\" action=\"/admin/elections/{{ELECTION_ID}}/voters\">\n"
    "      <div class=\"form-group\">\n"
    "        <label class=\"form-label\">Select Eligible Voters</label>\n"
    "        <p class=\"text-sm text-gray mb-3\">Only selected voters will be eligible to vote and apply as candidates</p>\n"
    "        <div class=\"card-glass p-4\" style=\"max-height:500px;overflow-y:auto;\">\n"
    "          <div class=\"mb-3\">\n"
    "            <button type=\"button\" class=\"btn btn-sm btn-secondary\" onclick=\"document.querySelectorAll('.voter-checkbox').forEach(c=>c.checked=true)\">Select All</button>\n"
    "            <button type=\"button\" class=\"btn btn-sm btn-secondary\" onclick=\"document.querySelectorAll('.voter-checkbox').forEach(c=>c.checked=false)\">Deselect All</button>\n"
    "          </div>\n"
    "          <div class=\"row g-2\">{{VOTER_CHECKBOXES}}</div>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3 mt-4\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary\">Update Voter List</button>\n"
    "        <a href=\"/admin/elections\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_MANAGE_VOTERS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Manage Voters - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><span style=\"color:var(--gray-600);\"><strong>{{USER_NAME}}</strong></span></li>\n"
    "        <li><a href=\"/admin/dashboard\">Dashboard</a></li>\n"
    "        <li><a href=\"/admin/elections\">Elections</a></li>\n"
    "        <li><a href=\"/admin/voters\">Voters</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"d-flex justify-content-between align-items-center mb-6\">\n"
    "    <h1>{{PAGE_HEADING}}</h1>\n"
    "    <a href=\"/admin/voters/add\" class=\"btn btn-primary\">➕ Add New Voter</a>\n"
    "  </div>\n"
    "  <!-- Search -->\n"
    "  <div class=\"card-glass p-4 mb-4\">\n"
    "    <form method=\"GET\" action=\"/admin/voters\">\n"
    "      <div class=\"row g-3\">\n"
    "        <div class=\"col-md-10\">\n"
    "          <input type=\"text\" name=\"search\" class=\"form-control\" placeholder=\"Search by name, CNIC, or email...\" value=\"{{SEARCH_QUERY}}\">\n"
    "        </div>\n"
    "        <div class=\"col-md-2\">\n"
    "          <button type=\"submit\" class=\"btn btn-secondary w-full\">🔍 Search</button>\n"
    "        </div>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "  <!-- Table -->\n"
    "  <div class=\"card-glass p-6\">\n"
    "    <h2 class=\"h4 mb-4\">Registered Voters ({{VOTER_COUNT}})</h2>\n"
    "    {{NO_VOTERS_MSG}}\n"
    "    <div class=\"table-responsive\">\n"
    "      <table class=\"table\">\n"
    "        <thead>\n"
    "          <tr><th>Name</th><th>CNIC</th><th>Email</th><th>Status</th><th>Registered</th><th>Actions</th></tr>\n"
    "        </thead>\n"
    "        <tbody>{{VOTER_ROWS}}</tbody>\n"
    "      </table>\n"
    "    </div>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ADMIN_PASSWORD_RESETS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Password Reset Requests - Admin</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/admin/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure Admin</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/admin/dashboard\">Dashboard</a></li>\n"
    "        <li><a href=\"/admin/voters\">Voters</a></li>\n"
    "        <li><a href=\"/admin/elections\">Elections</a></li>\n"
    "        <li><a href=\"/admin/password-resets\" class=\"btn btn-sm btn-primary\">🔑 Reset Requests</a></li>\n"
    "        <li><span style=\"color:var(--gray-500);font-size:var(--text-sm);\">👤 {{USER_NAME}}</span></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"d-flex justify-content-between align-items-center mb-6\">\n"
    "    <div>\n"
    "      <h1 class=\"mb-1\">🔑 Password Reset Requests</h1>\n"
    "      <p class=\"text-gray\">Review voter password reset requests and update their passwords.</p>\n"
    "    </div>\n"
    "    <a href=\"/admin/dashboard\" class=\"btn btn-outline\">← Dashboard</a>\n"
    "  </div>\n"
    "\n"
    "  <!-- Email Setup Notice -->\n"
    "  <div class=\"alert alert-info mb-4\">\n"
    "    <strong>📧 Email Setup:</strong> To send automatic email notifications, configure\n"
    "    <code>smtp_config.txt</code> in the root folder. Without it, the password is reset\n"
    "    but the email must be communicated manually to the voter.\n"
    "  </div>\n"
    "\n"
    "  <div class=\"card-glass p-6\">\n"
    "    {{RESET_TABLE}}\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert:not(.alert-info)');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_APPLICATION_STATUS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Application Status - Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/dashboard\">← Dashboard</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:700px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-2\">Application Status</h1>\n"
    "    <p class=\"text-gray mb-6\">Election: <strong>{{ELECTION_TITLE}}</strong></p>\n"
    "    <div class=\"text-center mb-6\">\n"
    "      <span class=\"badge badge-{{STATUS_COLOR}}\" style=\"font-size:var(--text-lg);padding:var(--space-3) var(--space-6);\">\n"
    "        {{APP_STATUS}}\n"
    "      </span>\n"
    "    </div>\n"
    "    {{REASON_HTML}}\n"
    "    <div class=\"card-glass p-4 mb-4\">\n"
    "      <h3 class=\"h5 mb-2\">Your Statement</h3>\n"
    "      <p class=\"text-gray\">{{APP_DESC}}</p>\n"
    "    </div>\n"
    "    <p class=\"text-sm text-gray\">Applied on: {{APP_DATE}}</p>\n"
    "    <div class=\"mt-6\">\n"
    "      <a href=\"/dashboard\" class=\"btn btn-outline\">← Back to Dashboard</a>\n"
    "    </div>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_APPLY_CANDIDATE =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Apply as Candidate - {{ELECTION_TITLE}}</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"></head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"/dashboard\">← Back to Dashboard</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"card-glass p-8\" style=\"max-width:700px;margin:0 auto;\">\n"
    "    <h1 class=\"h2 mb-2\">Apply as Candidate</h1>\n"
    "    <p class=\"text-gray mb-6\">Election: <strong>{{ELECTION_TITLE}}</strong></p>\n"
    "\n"
    "    <div class=\"card-glass p-4 mb-6\" style=\"background:var(--primary-50);border-left:4px solid var(--primary-500);\">\n"
    "      <p class=\"text-sm\"><strong>Your Name:</strong> {{USER_NAME}}</p>\n"
    "      <p class=\"text-sm mt-2\"><strong>Your CNIC:</strong> {{USER_CNIC_FMT}}</p>\n"
    "    </div>\n"
    "\n"
    "    <form method=\"POST\" action=\"/election/{{ELECTION_ID}}/apply\">\n"
    "      <div class=\"form-group\">\n"
    "        <label for=\"description\" class=\"form-label\">Why should voters choose you?</label>\n"
    "        <textarea id=\"description\" name=\"description\" class=\"form-control\" rows=\"5\"\n"
    "            placeholder=\"Describe your qualifications, experience, and what you plan to accomplish if elected... (minimum 10 characters)\" required></textarea>\n"
    "        <span class=\"form-text\">This will be shown to all eligible voters</span>\n"
    "      </div>\n"
    "      <div class=\"d-flex gap-3\">\n"
    "        <button type=\"submit\" class=\"btn btn-primary btn-lg\">Submit Application</button>\n"
    "        <a href=\"/dashboard\" class=\"btn btn-secondary\">Cancel</a>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_ELECTION_DETAILS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>{{ELECTION_TITLE}} - Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\">\n"
    "</head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"{{BACK_URL}}\" class=\"navbar-brand\">🗳️ VoteSecure</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"{{BACK_URL}}\">← Back</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "\n"
    "  <!-- Header -->\n"
    "  <div class=\"card-glass p-8 mb-8\">\n"
    "    <div class=\"d-flex justify-content-between align-items-start mb-4\">\n"
    "      <h1>{{ELECTION_TITLE}}</h1>\n"
    "      <span class=\"badge badge-{{STATUS_COLOR}}\" style=\"font-size:var(--text-base);padding:var(--space-2) var(--space-4);\">{{ELECTION_STATUS}}</span>\n"
    "    </div>\n"
    "    <p class=\"text-gray mb-6\">{{ELECTION_DESC}}</p>\n"
    "    <div style=\"display:flex;gap:var(--space-12);flex-wrap:wrap;\">\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-1\">Start Date</p>\n"
    "        <p class=\"fw-bold\">{{START_DATE}}</p>\n"
    "      </div>\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-1\">End Date</p>\n"
    "        <p class=\"fw-bold\">{{END_DATE}}</p>\n"
    "      </div>\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-1\">Total Votes</p>\n"
    "        <p class=\"fw-bold\" style=\"font-size:var(--text-2xl);color:var(--primary-600);\">{{TOTAL_VOTES}}</p>\n"
    "      </div>\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-1\">Candidates</p>\n"
    "        <p class=\"fw-bold\" style=\"font-size:var(--text-2xl);color:var(--secondary-600);\">{{TOTAL_CANDIDATES}}</p>\n"
    "      </div>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Alerts -->\n"
    "  {{ALERTS}}\n"
    "\n"
    "  <!-- Candidates -->\n"
    "  <div class=\"card-glass p-6 mb-6\">\n"
    "    <h2 class=\"h3 mb-6\">Candidates</h2>\n"
    "    {{CANDIDATES_HTML}}\n"
    "    {{NO_CANDIDATES}}\n"
    "  </div>\n"
    "\n"
    "  <div class=\"text-center mt-4\">\n"
    "    <a href=\"{{BACK_URL}}\" class=\"btn btn-outline\">← Back</a>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\">\n"
    "    <p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p>\n"
    "  </div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transform='translateY(-20px)';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  // Candidate card radio selection highlight\n"
    "  document.querySelectorAll('.candidate-card').forEach(card=>{\n"
    "    card.addEventListener('click',function(){\n"
    "      document.querySelectorAll('.candidate-card').forEach(c=>c.classList.remove('selected'));\n"
    "      this.classList.add('selected');\n"
    "      const r=this.querySelector('input[type=\"radio\"]');\n"
    "      if(r) r.checked=true;\n"
    "    });\n"
    "  });\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_FORGOT_PASSWORD =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Forgot Password - Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"><style>\n"
    "  body { min-height:100vh; display:flex; flex-direction:column; justify-content:center; background:linear-gradient(135deg,#0f172a 0%,#1e293b 50%,#0f172a 100%); }\n"
    "  .login-card { max-width:460px; margin:0 auto; width:100%; padding:var(--space-6); }\n"
    "  .brand { text-align:center; margin-bottom:var(--space-8); }\n"
    "  .brand-icon { font-size:3rem; display:block; margin-bottom:var(--space-2); }\n"
    "  .brand-title { font-size:var(--text-3xl); font-weight:800; background:var(--gradient-primary); -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text; }\n"
    "  .brand-sub { color:rgba(255,255,255,0.5); font-size:var(--text-sm); margin-top:var(--space-1); }\n"
    "  .card-glass { background:rgba(255,255,255,0.07); border:1px solid rgba(255,255,255,0.12); border-radius:var(--radius-2xl); padding:var(--space-8); }\n"
    "  .card-glass .form-label { color:rgba(255,255,255,0.8); }\n"
    "  .card-glass h2 { color:#fff; }\n"
    "  .card-glass p { color:rgba(255,255,255,0.5); }\n"
    "  .form-control { background:rgba(255,255,255,0.05); border-color:rgba(255,255,255,0.15); color:#fff; }\n"
    "  .form-control:focus { background:rgba(255,255,255,0.1); border-color:var(--primary-500); color:#fff; }\n"
    "  .form-control::placeholder { color:rgba(255,255,255,0.3); }\n"
    "  .info-box { background:rgba(99,102,241,0.15); border:1px solid rgba(99,102,241,0.3); border-radius:var(--radius-lg); padding:var(--space-4); margin-bottom:var(--space-6); color:rgba(255,255,255,0.8); font-size:var(--text-sm); }\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<div class=\"container\">\n"
    "  <div class=\"login-card py-8\">\n"
    "    <div class=\"brand\">\n"
    "      <span class=\"brand-icon\">🔑</span>\n"
    "      <div class=\"brand-title\">VoteSecure</div>\n"
    "      <div class=\"brand-sub\">Online Voting System</div>\n"
    "    </div>\n"
    "    {{FLASH_MESSAGE}}\n"
    "    <div class=\"card-glass\">\n"
    "      <h2 class=\"mb-2\">Forgot Password?</h2>\n"
    "      <p class=\"mb-6\">Enter your CNIC and registered email address to submit a reset request.</p>\n"
    "      <div class=\"info-box\">\n"
    "        📧 Your request will be reviewed by the admin. You will receive your new password via your registered email address.\n"
    "      </div>\n"
    "      <form method=\"POST\" action=\"/forgot-password\">\n"
    "        <div class=\"form-group\">\n"
    "          <label for=\"cnic\" class=\"form-label\">CNIC Number</label>\n"
    "          <input type=\"text\" id=\"cnic\" name=\"cnic\" class=\"form-control\"\n"
    "            placeholder=\"0000000000000 (13 digits)\" required maxlength=\"15\"\n"
    "            pattern=\"\\\\d{13}\" title=\"Enter 13 digit CNIC without dashes\">\n"
    "        </div>\n"
    "        <div class=\"form-group\">\n"
    "          <label for=\"email\" class=\"form-label\">Registered Email</label>\n"
    "          <input type=\"email\" id=\"email\" name=\"email\" class=\"form-control\"\n"
    "            placeholder=\"your@email.com\" required>\n"
    "        </div>\n"
    "        <button type=\"submit\" class=\"btn btn-primary w-full btn-lg mb-4\">\n"
    "          📤 Submit Reset Request\n"
    "        </button>\n"
    "        <div class=\"text-center\">\n"
    "          <a href=\"/login\" style=\"color:rgba(255,255,255,0.5);font-size:var(--text-sm);\">← Back to Login</a>\n"
    "        </div>\n"
    "      </form>\n"
    "    </div>\n"
    "  </div>\n"
    "</div>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},7000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  /* Format CNIC input */\n"
    "  const cnic=document.getElementById('cnic');\n"
    "  if(cnic){cnic.addEventListener('input',function(){this.value=this.value.replace(/\\\\D/g,'').slice(0,13);});}\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_LOGIN =
    "<!-- SHARED BASE LAYOUT - included in all pages via C template engine -->\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "<meta name=\"description\" content=\"Secure Online Voting System - Cast your vote with confidence\">\n"
    "<meta name=\"author\" content=\"Anas Farooq\">\n"
    "<title>Login - Online Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\">\n"
    "</head>\n"
    "<body>\n"
    "<div style=\"min-height:100vh;display:flex;align-items:center;justify-content:center;background:linear-gradient(135deg,var(--primary-50) 0%,var(--secondary-50) 100%);padding:var(--space-6);\">\n"
    "<div style=\"width:100%;max-width:480px;\">\n"
    "\n"
    "    <!-- Logo -->\n"
    "    <div class=\"text-center mb-8\">\n"
    "        <div style=\"font-size:4rem;margin-bottom:var(--space-4);\">🗳️</div>\n"
    "        <h1 style=\"background:var(--gradient-primary);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;margin-bottom:var(--space-2);\">VoteSecure</h1>\n"
    "        <p class=\"text-gray\">Online Voting System</p>\n"
    "    </div>\n"
    "\n"
    "    <!-- Flash Message -->\n"
    "    {{FLASH_MESSAGE}}\n"
    "\n"
    "    <!-- Login Card -->\n"
    "    <div class=\"card-glass p-8\">\n"
    "        <h2 class=\"text-center mb-6\" style=\"font-size:var(--text-2xl);\">Sign In</h2>\n"
    "\n"
    "        <form method=\"POST\" action=\"/login\" id=\"loginForm\">\n"
    "            <div class=\"form-group\">\n"
    "                <label for=\"identifier\" class=\"form-label\">CNIC (Voters) or Email (Admin)</label>\n"
    "                <input type=\"text\" id=\"identifier\" name=\"identifier\" class=\"form-control\"\n"
    "                    placeholder=\"00000-0000000-0 or admin@email.com\"\n"
    "                    value=\"{{IDENTIFIER_VAL}}\" required autocomplete=\"username\">\n"
    "                <span class=\"form-text\">Voters use CNIC number, Admins use email address</span>\n"
    "            </div>\n"
    "            <div class=\"form-group\">\n"
    "                <label for=\"password\" class=\"form-label\">Password</label>\n"
    "                <input type=\"password\" id=\"password\" name=\"password\" class=\"form-control\"\n"
    "                    placeholder=\"Enter your password\" required autocomplete=\"current-password\">\n"
    "            </div>\n"
    "            <button type=\"submit\" class=\"btn btn-primary w-full btn-lg\" id=\"signinBtn\">\n"
    "                🔐 Sign In\n"
    "            </button>\n"
    "            <div class=\"text-center mt-4\">\n"
    "                <a href=\"/forgot-password\" style=\"color:var(--primary-400);font-size:var(--text-sm);\">\n"
    "                    🔑 Forgot your password?\n"
    "                </a>\n"
    "            </div>\n"
    "        </form>\n"
    "    </div>\n"
    "\n"
    "    <!-- Footer note -->\n"
    "    <p class=\"text-center text-gray text-sm mt-6\">\n"
    "        &copy; 2026 Online Voting System &mdash; Developed by <strong>Anas Farooq</strong><br>\n"
    "        <span style=\"color:var(--gray-500);\">BS Computer Science | Ziauddin University</span>\n"
    "    </p>\n"
    "</div>\n"
    "</div>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded', function() {\n"
    "    const alerts = document.querySelectorAll('.alert');\n"
    "    alerts.forEach(a => {\n"
    "        setTimeout(() => {\n"
    "            a.style.opacity = '0';\n"
    "            a.style.transform = 'translateY(-20px)';\n"
    "            a.style.transition = 'all 0.3s';\n"
    "            setTimeout(() => a.remove(), 300);\n"
    "        }, 5000);\n"
    "    });\n"
    "    // Clear flash cookies\n"
    "    document.cookie = 'flash_msg=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "    document.cookie = 'flash_type=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_RESULTS =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<title>Results - {{ELECTION_TITLE}}</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\"><style>\n"
    "@keyframes confetti{0%{transform:translateY(-100vh) rotate(0deg);}100%{transform:translateY(100vh) rotate(720deg);}}\n"
    "</style>\n"
    "</head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"{{BACK_URL}}\" class=\"navbar-brand\">🗳️ VoteSecure</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><a href=\"{{BACK_URL}}\">← Back</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-8\">\n"
    "  <!-- Header -->\n"
    "  <div class=\"card-glass mb-8\" style=\"padding:var(--space-8);\">\n"
    "    <div class=\"text-center mb-6\">\n"
    "      <h1 style=\"margin-bottom:var(--space-3);\">🏆 Election Results</h1>\n"
    "      <h2 class=\"text-primary\">{{ELECTION_TITLE}}</h2>\n"
    "      <p class=\"text-gray mt-3\">{{ELECTION_DESC}}</p>\n"
    "    </div>\n"
    "    <div style=\"display:flex;justify-content:center;gap:var(--space-12);margin-top:var(--space-8);flex-wrap:wrap;\">\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-2\">Total Votes Cast</p>\n"
    "        <p class=\"fw-bold\" style=\"font-size:var(--text-3xl);color:var(--primary-600);\">{{TOTAL_VOTES}}</p>\n"
    "      </div>\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-2\">Total Candidates</p>\n"
    "        <p class=\"fw-bold\" style=\"font-size:var(--text-3xl);color:var(--secondary-600);\">{{TOTAL_CANDIDATES}}</p>\n"
    "      </div>\n"
    "      <div class=\"text-center\">\n"
    "        <p class=\"text-sm text-gray mb-2\">Election Status</p>\n"
    "        <span class=\"badge badge-{{STATUS_COLOR}}\" style=\"font-size:var(--text-base);padding:var(--space-2) var(--space-4);\">{{ELECTION_STATUS}}</span>\n"
    "      </div>\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <!-- Winner -->\n"
    "  {{WINNER_HTML}}\n"
    "\n"
    "  <!-- Results list -->\n"
    "  {{NO_RESULTS}}\n"
    "  <div class=\"card\" style=\"padding:var(--space-8);\">\n"
    "    <h2 class=\"mb-6\">Complete Results</h2>\n"
    "    <div style=\"display:flex;flex-direction:column;gap:var(--space-6);\">\n"
    "      {{RESULTS_ROWS}}\n"
    "    </div>\n"
    "  </div>\n"
    "\n"
    "  <div class=\"text-center mt-8\">\n"
    "    <a href=\"{{BACK_URL}}\" class=\"btn btn-outline\">← Back</a>\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\"><p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p></div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  setTimeout(function(){\n"
    "    document.querySelectorAll('.progress-bar').forEach(bar=>{\n"
    "      const tw=parseFloat(bar.getAttribute('data-width'))||0;\n"
    "      bar.style.width='0%';\n"
    "      let w=0;\n"
    "      const iv=setInterval(()=>{\n"
    "        if(w>=tw){clearInterval(iv);}\n"
    "        else{w=Math.min(w+1,tw);bar.style.width=w+'%';}\n"
    "      },10);\n"
    "    });\n"
    "  },300);\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *TPL_VOTER_DASHBOARD =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "<meta name=\"description\" content=\"Voter Dashboard - Online Voting System\">\n"
    "<title>My Elections - Voting System</title>\n"
    "<link rel=\"stylesheet\" href=\"/style.css\">\n"
    "</head>\n"
    "<body>\n"
    "<nav class=\"navbar\">\n"
    "  <div class=\"container\">\n"
    "    <div class=\"navbar-content\">\n"
    "      <a href=\"/dashboard\" class=\"navbar-brand\">🗳️ VoteSecure</a>\n"
    "      <ul class=\"navbar-nav\">\n"
    "        <li><span style=\"color:var(--gray-600);\">Welcome, <strong>{{USER_NAME}}</strong></span></li>\n"
    "        <li><a href=\"/dashboard\">My Elections</a></li>\n"
    "        <li><a href=\"/logout\" class=\"btn btn-sm btn-outline\">Logout</a></li>\n"
    "      </ul>\n"
    "    </div>\n"
    "  </div>\n"
    "</nav>\n"
    "<div class=\"container mt-4\">{{FLASH_MESSAGE}}</div>\n"
    "<main>\n"
    "<div class=\"container py-6\">\n"
    "  <div class=\"mb-8\">\n"
    "    <h1>My Elections</h1>\n"
    "    <p class=\"lead text-gray\">View and participate in your eligible elections</p>\n"
    "    <p class=\"text-sm text-gray mt-2\">Your CNIC: <strong>{{USER_CNIC}}</strong></p>\n"
    "  </div>\n"
    "  <div class=\"row g-4\">\n"
    "    {{ELECTION_CARDS}}\n"
    "  </div>\n"
    "</div>\n"
    "</main>\n"
    "<footer class=\"footer\">\n"
    "  <div class=\"container\">\n"
    "    <p>&copy; 2026 Online Voting System | Developed by <strong>Anas Farooq</strong></p>\n"
    "    <p class=\"text-sm\" style=\"margin-top:var(--space-2);\">BS Computer Science | Ziauddin University</p>\n"
    "    <p class=\"text-sm\" style=\"color:var(--gray-500);margin-top:var(--space-2);\">Secure &bull; Transparent &bull; Reliable</p>\n"
    "  </div>\n"
    "</footer>\n"
    "<script src=\"/app.js\"></script>\n"
    "<script>\n"
    "document.addEventListener('DOMContentLoaded',function(){\n"
    "  const alerts=document.querySelectorAll('.alert');\n"
    "  alerts.forEach(a=>{setTimeout(()=>{a.style.opacity='0';a.style.transform='translateY(-20px)';a.style.transition='all 0.3s';setTimeout(()=>a.remove(),300)},5000);});\n"
    "  document.cookie='flash_msg=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "  document.cookie='flash_type=;Path=/;Expires=Thu, 01 Jan 1970 00:00:01 GMT;';\n"
    "});\n"
    "</script>\n"
    "</body>\n"
    "</html>\n"
    "";

static const char *get_embedded_template(const char *name) {
    if (strcmp(name, "admin_add_voter.html") == 0) return TPL_ADMIN_ADD_VOTER;
    if (strcmp(name, "admin_applications.html") == 0) return TPL_ADMIN_APPLICATIONS;
    if (strcmp(name, "admin_change_password.html") == 0) return TPL_ADMIN_CHANGE_PASSWORD;
    if (strcmp(name, "admin_create_election.html") == 0) return TPL_ADMIN_CREATE_ELECTION;
    if (strcmp(name, "admin_dashboard.html") == 0) return TPL_ADMIN_DASHBOARD;
    if (strcmp(name, "admin_edit_election.html") == 0) return TPL_ADMIN_EDIT_ELECTION;
    if (strcmp(name, "admin_edit_voter.html") == 0) return TPL_ADMIN_EDIT_VOTER;
    if (strcmp(name, "admin_election_applications.html") == 0) return TPL_ADMIN_ELECTION_APPLICATIONS;
    if (strcmp(name, "admin_elections.html") == 0) return TPL_ADMIN_ELECTIONS;
    if (strcmp(name, "admin_manage_candidates.html") == 0) return TPL_ADMIN_MANAGE_CANDIDATES;
    if (strcmp(name, "admin_manage_election_voters.html") == 0) return TPL_ADMIN_MANAGE_ELECTION_VOTERS;
    if (strcmp(name, "admin_manage_voters.html") == 0) return TPL_ADMIN_MANAGE_VOTERS;
    if (strcmp(name, "admin_password_resets.html") == 0) return TPL_ADMIN_PASSWORD_RESETS;
    if (strcmp(name, "application_status.html") == 0) return TPL_APPLICATION_STATUS;
    if (strcmp(name, "apply_candidate.html") == 0) return TPL_APPLY_CANDIDATE;
    if (strcmp(name, "election_details.html") == 0) return TPL_ELECTION_DETAILS;
    if (strcmp(name, "forgot_password.html") == 0) return TPL_FORGOT_PASSWORD;
    if (strcmp(name, "login.html") == 0) return TPL_LOGIN;
    if (strcmp(name, "results.html") == 0) return TPL_RESULTS;
    if (strcmp(name, "voter_dashboard.html") == 0) return TPL_VOTER_DASHBOARD;
    return NULL;
}


/* ================================================================
   API HANDLERS DECLARATIONS
   ================================================================ */



/* Utility */
void send_redirect(struct mg_connection *c, const char *location);
void send_html_page(struct mg_connection *c, const char *path,
                    const char **keys, const char **vals, int kv_count);
void set_flash(char *flash_store, const char *msg, const char *type);
const char *get_cookie(const char *cookie_hdr, const char *name, char *out, int max);

/* Route Handlers */
void handle_login_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_login_post(struct mg_connection *c, struct mg_http_message *hm);
void handle_logout(struct mg_connection *c, struct mg_http_message *hm);
void handle_voter_dashboard(struct mg_connection *c, struct mg_http_message *hm);
void handle_election_details(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_cast_vote(struct mg_connection *c, struct mg_http_message *hm);
void handle_apply_get(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_apply_post(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_view_application(struct mg_connection *c, struct mg_http_message *hm, int app_id);
void handle_public_results(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_dashboard(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_voters(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_add_voter_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_add_voter_post(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_edit_voter_get(struct mg_connection *c, struct mg_http_message *hm, int uid);
void handle_admin_edit_voter_post(struct mg_connection *c, struct mg_http_message *hm, int uid);
void handle_admin_ban_voter(struct mg_connection *c, struct mg_http_message *hm, int uid);
void handle_admin_unban_voter(struct mg_connection *c, struct mg_http_message *hm, int uid);
void handle_admin_delete_voter_permanent(struct mg_connection *c, struct mg_http_message *hm, int uid);
void handle_admin_elections(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_create_election_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_create_election_post(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_edit_election_get(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_edit_election_post(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_election_voters_get(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_election_voters_post(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_delete_election(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_applications(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_election_applications(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_approve_application(struct mg_connection *c, struct mg_http_message *hm, int app_id);
void handle_admin_reject_application(struct mg_connection *c, struct mg_http_message *hm, int app_id);
void handle_admin_manage_candidates(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_add_candidate(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_delete_candidate(struct mg_connection *c, struct mg_http_message *hm, int cid);
void handle_admin_results(struct mg_connection *c, struct mg_http_message *hm, int eid);
void handle_admin_change_password_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_change_password_post(struct mg_connection *c, struct mg_http_message *hm);

/* Session helpers exposed to server.c */
Session get_session_from_request(struct mg_http_message *hm);

/* Forgot password / password reset */
void handle_forgot_password_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_forgot_password_post(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_password_resets(struct mg_connection *c, struct mg_http_message *hm);
void handle_admin_resolve_reset(struct mg_connection *c, struct mg_http_message *hm, int req_id);


/* ================================================================
   API HANDLERS IMPLEMENTATION
   ================================================================ */


/* ================================================================
   SHARED FLASH MESSAGE STORAGE  (per-request, simple ring buffer)
   ================================================================ */
static char g_flash_msg[512]  = "";
static char g_flash_type[32]  = "info";

void set_flash(char *flash_store, const char *msg, const char *type) {
    (void)flash_store;
    strncpy(g_flash_msg,  msg  ? msg  : "", 511);
    strncpy(g_flash_type, type ? type : "info", 31);
}

static void flush_flash(void) {
    g_flash_msg[0] = '\0';
    g_flash_type[0] = '\0';
}

/* ================================================================
   COOKIE HELPERS
   ================================================================ */
const char *get_cookie(const char *cookie_hdr, const char *name, char *out, int max) {
    if (!cookie_hdr) { out[0]=0; return NULL; }
    char search[64];
    snprintf(search, sizeof(search), "%s=", name);
    const char *p = strstr(cookie_hdr, search);
    if (!p) { out[0]=0; return NULL; }
    p += strlen(search);
    int i = 0;
    while (*p && *p != ';' && i < max-1) out[i++] = *p++;
    out[i] = 0;
    return out;
}

Session get_session_from_request(struct mg_http_message *hm) {
    struct mg_str *cookie_hdr = mg_http_get_header(hm, "Cookie");
    char sid[SESSION_ID_LEN + 1] = "";
    if (cookie_hdr) {
        char buf[256] = "";
        char tmp[cookie_hdr->len + 1];
        memcpy(tmp, cookie_hdr->buf, cookie_hdr->len);
        tmp[cookie_hdr->len] = '\0';
        get_cookie(tmp, SESSION_COOKIE_NAME, buf, sizeof(buf));
        strncpy(sid, buf, SESSION_ID_LEN);
    }
    return session_get(db_get(), sid);
}

/* ================================================================
   HTML TEMPLATE ENGINE (lightweight string replacement)
   ================================================================ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/* Replace all occurrences of {{key}} with val in template src */
static char *str_replace(const char *src, const char *key, const char *val) {
    if (!src || !key || !val) return src ? strdup(src) : NULL;
    char placeholder[128];
    snprintf(placeholder, sizeof(placeholder), "{{%s}}", key);
    size_t plen = strlen(placeholder);
    size_t vlen = strlen(val);
    size_t slen = strlen(src);

    /* Count occurrences */
    int cnt = 0;
    const char *p = src;
    while ((p = strstr(p, placeholder))) { cnt++; p += plen; }
    if (cnt == 0) return strdup(src);

    size_t newsz = slen + (vlen - plen) * cnt + 1;
    char  *out   = malloc(newsz);
    if (!out) return strdup(src);

    char *dst = out;
    p = src;
    while (*p) {
        if (strncmp(p, placeholder, plen) == 0) {
            memcpy(dst, val, vlen);
            dst += vlen;
            p   += plen;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    return out;
}

/* Load page, apply flash + multiple kv replacements, send */
void send_html_page(struct mg_connection *c, const char *path,
                    const char **keys, const char **vals, int kv_count) {
    char full_path[512];
    (void)full_path; /* templates are embedded */

    const char *tpl_src = get_embedded_template(path);
    char *tmpl = tpl_src ? strdup(tpl_src) : NULL;
    if (!tmpl) {
        mg_http_reply(c, 404, "", "Page not found: %s", path);
        return;
    }

    /* Inject flash */
    char flash_html[700] = "";
    if (g_flash_msg[0]) {
        snprintf(flash_html, sizeof(flash_html),
            "<div class=\"alert alert-%s\" id=\"flash-msg\">%s</div>",
            g_flash_type, g_flash_msg);
    }

    /* Apply replacements */
    char *cur = str_replace(tmpl, "FLASH_MESSAGE", flash_html);
    free(tmpl);
    flush_flash();

    for (int i = 0; i < kv_count; i++) {
        char *next = str_replace(cur, keys[i], vals[i]);
        free(cur);
        cur = next;
    }

    mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", cur);
    free(cur);
}

void send_redirect(struct mg_connection *c, const char *location) {
    mg_http_reply(c, 302, "Location: %s\r\n", "", location);
}

/* ================================================================
   FORM PARSING HELPERS
   ================================================================ */
static int form_get(struct mg_http_message *hm, const char *key, char *out, int max) {
    struct mg_str val = mg_http_var(hm->body, mg_str(key));
    if (val.len == 0) { out[0]=0; return 0; }
    int n = val.len < (size_t)(max-1) ? (int)val.len : max-1;
    strncpy(out, val.buf, n);
    out[n] = 0;
    /* URL-decode '+' as space */
    for (int i=0;i<n;i++) if(out[i]=='+') out[i]=' ';
    mg_url_decode(out, strlen(out), out, max, 1);
    return 1;
}

static void clean_cnic(char *cnic) {
    char tmp[32]=""; int j=0;
    for (int i=0; cnic[i] && j<13; i++)
        if (isdigit((unsigned char)cnic[i])) tmp[j++]=cnic[i];
    tmp[j]=0;
    strncpy(cnic, tmp, 32);
}

static void format_cnic(const char *cnic, char *out) {
    if (strlen(cnic) == 13) {
        snprintf(out, 20, "%.5s-%.7s-%.1s", cnic, cnic+5, cnic+12);
    } else {
        strncpy(out, cnic, 20);
    }
}


/* Read flash from cookies */
static void read_flash_cookies(struct mg_http_message *hm) {
    struct mg_str *ch = mg_http_get_header(hm, "Cookie");
    if (!ch) return;
    char tmp[ch->len+1];
    memcpy(tmp, ch->buf, ch->len);
    tmp[ch->len]=0;
    char msg[512]="", type[32]="";
    get_cookie(tmp, "flash_msg",  msg,  sizeof(msg));
    get_cookie(tmp, "flash_type", type, sizeof(type));
    if (msg[0]) {
        /* URL-decode: converts %20 -> space, %2C -> comma, etc. */
        char decoded[512]="";
        mg_url_decode(msg, strlen(msg), decoded, sizeof(decoded)-1, 1);
        strncpy(g_flash_msg,  decoded[0] ? decoded : msg, 511);
        strncpy(g_flash_type, type[0] ? type : "info", 31);
    }
}

/* Send redirect WITH flash cookies set */
static void redirect_with_flash(struct mg_connection *c, const char *location,
                                const char *msg, const char *type) {
    char enc_msg[1024]="";
    /* Simple encoding: replace spaces with %20 */
    int j=0;
    for (int i=0;msg[i]&&j<1020;i++){
        if(msg[i]==' '){ enc_msg[j++]='%';enc_msg[j++]='2';enc_msg[j++]='0';}
        else if(msg[i]=='"'){ enc_msg[j++]='%';enc_msg[j++]='2';enc_msg[j++]='2';}
        else enc_msg[j++]=msg[i];
    }
    enc_msg[j]=0;
    char hdr[1200];
    snprintf(hdr, sizeof(hdr),
        "Location: %s\r\n"
        "Set-Cookie: flash_msg=%s; Path=/\r\n"
        "Set-Cookie: flash_type=%s; Path=/\r\n",
        location, enc_msg, type);
    mg_http_reply(c, 302, hdr, "");
}

/* ================================================================
   AUTH ROUTES
   ================================================================ */

void handle_login_get(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    /* If already logged in, redirect */
    Session s = get_session_from_request(hm);
    if (s.valid) {
        send_redirect(c, s.is_admin ? "/admin/dashboard" : "/dashboard");
        return;
    }
    const char *k[] = {"IDENTIFIER_VAL"};
    const char *v[] = {""};
    send_html_page(c, "login.html", k, v, 1);
}

void handle_login_post(struct mg_connection *c, struct mg_http_message *hm) {
    char identifier[128]="", password[128]="";
    form_get(hm, "identifier", identifier, sizeof(identifier));
    form_get(hm, "password",   password,   sizeof(password));

    if (!identifier[0] || !password[0]) {
        redirect_with_flash(c, "/login", "Please provide both CNIC/Email and password.", "danger");
        return;
    }

    char hashed[65];
    sha256_string(password, hashed);

    int uid=0, is_admin=0, is_deleted=0, found=0;

    if (strchr(identifier, '@')) {
        /* Email login â€” admin only */
        found = db_verify_user_by_email(identifier, hashed, &uid, &is_admin, &is_deleted);
        if (found && !is_admin) {
            redirect_with_flash(c, "/login",
                "Voters must login using CNIC, not email.", "warning");
            return;
        }
        if (!found) {
            redirect_with_flash(c, "/login",
                "Admin email not found. Please check your credentials.", "danger");
            return;
        }
    } else {
        /* CNIC login â€” voters only */
        char cnic[32]; strncpy(cnic, identifier, 31); cnic[31]=0;
        clean_cnic(cnic);
        if (strlen(cnic) != 13) {
            redirect_with_flash(c, "/login",
                "Invalid CNIC format. CNIC must be 13 digits.", "danger");
            return;
        }
        found = db_verify_user_by_cnic(cnic, hashed, &uid, &is_admin, &is_deleted);
        if (!found) {
            redirect_with_flash(c, "/login",
                "You are not a registered voter. Please contact your administrator.", "warning");
            return;
        }
    }

    if (is_deleted) {
        redirect_with_flash(c, "/login",
            "This account has been banned. Please contact the administrator.", "danger");
        return;
    }

    /* Get user details */
    char name[128]="", cnic_str[20]="", email[120]="";
    int dummy1, dummy2; char dummy3[32];
    db_get_user(uid, name, cnic_str, email, &dummy1, &dummy2, dummy3);

    /* Create session */
    char sid[SESSION_ID_LEN+1]="";
    session_create(db_get(), uid, name, cnic_str, email, is_admin, sid);

    /* Set cookie and redirect */
    char cookie_hdr[256];
    snprintf(cookie_hdr, sizeof(cookie_hdr),
        "Set-Cookie: %s=%s; Path=/; HttpOnly\r\n"
        "Set-Cookie: flash_msg=Welcome%%20back%%2C%%20%s%%21; Path=/\r\n"
        "Set-Cookie: flash_type=success; Path=/\r\n",
        SESSION_COOKIE_NAME, sid, name);

    char full_hdr[512];
    snprintf(full_hdr, sizeof(full_hdr),
        "Set-Cookie: %s=%s; Path=/; HttpOnly\r\n"
        "Set-Cookie: flash_msg=Welcome%%20back%%2C%%20%s%%21; Path=/\r\n"
        "Set-Cookie: flash_type=success; Path=/\r\n"
        "Location: %s\r\n",
        SESSION_COOKIE_NAME, sid, name,
        is_admin ? "/admin/dashboard" : "/dashboard");
    mg_http_reply(c, 302, full_hdr, "");
}

void handle_logout(struct mg_connection *c, struct mg_http_message *hm) {
    Session s = get_session_from_request(hm);
    if (s.valid) session_destroy(db_get(), s.session_id);
    char logout_hdr[512];
    snprintf(logout_hdr, sizeof(logout_hdr),
        "Set-Cookie: %s=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n"
        "Set-Cookie: flash_msg=You%%20have%%20been%%20logged%%20out.; Path=/\r\n"
        "Set-Cookie: flash_type=success; Path=/\r\n"
        "Location: /login\r\n",
        SESSION_COOKIE_NAME);
    mg_http_reply(c, 302, logout_hdr, "");
}

/* ================================================================
   VOTER DASHBOARD
   ================================================================ */

void handle_voter_dashboard(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }
    if (s.is_admin) { send_redirect(c, "/admin/dashboard"); return; }

    db_update_election_statuses();

    /* Query all non-deleted elections */
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT election_id, title, description, status, start_date, end_date "
        "FROM elections WHERE is_deleted=0 ORDER BY created_at DESC;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    /* Build election cards HTML */
    char cards[65536] = "";
    char row[4096];

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int eid    = sqlite3_column_int(stmt, 0);
        const char *title  = (const char*)sqlite3_column_text(stmt, 1);
        const char *desc   = (const char*)sqlite3_column_text(stmt, 2);
        const char *status = (const char*)sqlite3_column_text(stmt, 3);
        const char *sdate  = (const char*)sqlite3_column_text(stmt, 4);
        const char *edate  = (const char*)sqlite3_column_text(stmt, 5);

        /* Determine badges */
        const char *stat_col = "secondary";
        if (strcmp(status,"active")==0) stat_col="success";
        else if (strcmp(status,"upcoming")==0) stat_col="warning";
        else stat_col="secondary";

        int eligible = db_is_eligible(eid, s.user_id);
        int has_voted = db_user_has_voted(s.user_id, eid);
        int has_applied = db_user_has_applied(s.user_id, eid);
        int is_cand = db_user_is_candidate(s.user_id, eid);
        int total_candidates=0;
        {
            sqlite3_stmt *cs;
            char csql[128]; snprintf(csql,128,"SELECT COUNT(*) FROM candidates WHERE election_id=%d;",eid);
            sqlite3_prepare_v2(db_get(),csql,-1,&cs,NULL);
            if(sqlite3_step(cs)==SQLITE_ROW) total_candidates=sqlite3_column_int(cs,0);
            sqlite3_finalize(cs);
        }

        char desc_short[104]="";
        if (desc) { strncpy(desc_short, desc, 100); if(strlen(desc)>100) strcpy(desc_short+100,"..."); }

        char apply_btn[256]="", voted_alert[256]="", cand_alert[256]="", app_alert[256]="", results_btn[256]="";

        if (has_voted) snprintf(voted_alert,sizeof(voted_alert),
            "<div class=\"alert alert-success p-2 mb-3\">âœ… You have voted in this election</div>");
        if (is_cand)   snprintf(cand_alert,sizeof(cand_alert),
            "<div class=\"alert alert-info p-2 mb-3\">â„¹ï¸ You are a candidate in this election</div>");
        if (has_applied && !is_cand) {
            /* get status */
            sqlite3_stmt *as2;
            char asql[200]; snprintf(asql,200,"SELECT status FROM candidate_applications WHERE user_id=%d AND election_id=%d;",s.user_id,eid);
            sqlite3_prepare_v2(db_get(),asql,-1,&as2,NULL);
            char ast[20]="pending";
            if(sqlite3_step(as2)==SQLITE_ROW){ const char *t=(const char*)sqlite3_column_text(as2,0); if(t)strncpy(ast,t,19); }
            sqlite3_finalize(as2);
            const char *ac=strcmp(ast,"pending")==0?"warning":(strcmp(ast,"approved")==0?"success":"danger");
            snprintf(app_alert,sizeof(app_alert),
                "<div class=\"alert alert-%s p-2 mb-3\">Application: %s</div>",ac,ast);
        }
        if (eligible && strcmp(status,"upcoming")==0 && !has_applied && !is_cand)
            snprintf(apply_btn,sizeof(apply_btn),
                "<a href=\"/election/%d/apply\" class=\"btn btn-secondary btn-sm\">Apply as Candidate</a>", eid);
        if (strcmp(status,"closed")==0)
            snprintf(results_btn,sizeof(results_btn),
                "<a href=\"/results/%d\" class=\"btn btn-success btn-sm\">View Results</a>", eid);

        snprintf(row, sizeof(row),
            "<div class=\"col-md-6 col-lg-4\">"
            "<div class=\"card-glass p-6 h-100\">"
            "<div class=\"d-flex justify-content-between align-items-start mb-3\">"
            "<h2 class=\"h4 mb-0\">%s</h2>"
            "<div class=\"d-flex flex-column gap-2\">"
            "<span class=\"badge badge-%s\">%s</span>"
            "<span class=\"badge badge-%s\">%s</span>"
            "</div></div>"
            "<p class=\"text-gray mb-4\">%s</p>"
            "<div class=\"mb-4\">"
            "<small class=\"text-gray d-block\">ðŸ“… %s</small>"
            "<small class=\"text-gray d-block\">ðŸ %s</small>"
            "<small class=\"text-gray d-block mt-2\">ðŸ‘¥ %d candidates</small>"
            "</div>"
            "%s%s%s"
            "<div class=\"d-flex gap-2\">"
            "<a href=\"/election/%d\" class=\"btn btn-primary btn-sm\">View Details</a>"
            "%s%s"
            "</div>"
            "</div></div>",
            title ? title : "",
            stat_col, status ? status : "",
            eligible ? "success":"danger", eligible ? "âœ“ ELIGIBLE":"âœ— NOT ELIGIBLE",
            desc_short,
            sdate ? sdate : "", edate ? edate : "",
            total_candidates,
            voted_alert, cand_alert, app_alert,
            eid, apply_btn, results_btn
        );
        strncat(cards, row, sizeof(cards)-strlen(cards)-1);
    }
    sqlite3_finalize(stmt);

    if (!cards[0]) {
        strncpy(cards,
            "<div class=\"col-12\"><div class=\"card-glass p-8 text-center\">"
            "<h2 class=\"h3 mb-3\">No Elections Available</h2>"
            "<p class=\"text-gray\">You are not currently eligible to participate in any elections.</p>"
            "</div></div>", sizeof(cards));
    }

    char cnic_fmt[20]; format_cnic(s.user_cnic, cnic_fmt);
    const char *k[] = {"USER_NAME", "USER_CNIC", "ELECTION_CARDS"};
    const char *v[] = {s.user_name, cnic_fmt, cards};
    send_html_page(c, "voter_dashboard.html", k, v, 3);
}

/* ================================================================
   ELECTION DETAILS + VOTING
   ================================================================ */

void handle_election_details(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    db_update_election_statuses();

    /* Get election */
    sqlite3_stmt *stmt;
    const char *esql =
        "SELECT title, description, status, start_date, end_date FROM elections "
        "WHERE election_id=? AND is_deleted=0;";
    sqlite3_prepare_v2(db_get(), esql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, eid);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        redirect_with_flash(c, s.is_admin?"/admin/elections":"/dashboard","Election not found.","danger");
        return;
    }
    char title[256]="", desc[2048]="", status[20]="", sdate[32]="", edate[32]="";
    const char *t=(const char*)sqlite3_column_text(stmt,0);
    const char *d=(const char*)sqlite3_column_text(stmt,1);
    const char *st=(const char*)sqlite3_column_text(stmt,2);
    const char *sd=(const char*)sqlite3_column_text(stmt,3);
    const char *ed=(const char*)sqlite3_column_text(stmt,4);
    if(t) strncpy(title,t,255); if(d) strncpy(desc,d,2047);
    if(st) strncpy(status,st,19); if(sd) strncpy(sdate,sd,31); if(ed) strncpy(edate,ed,31);
    sqlite3_finalize(stmt);

    int eligible  = db_is_eligible(eid, s.user_id);
    int has_voted = db_user_has_voted(s.user_id, eid);
    int total_votes = db_get_total_votes_in_election(eid);

    /* Alerts */
    char alerts[1024]="";
    if (has_voted) {
        strncat(alerts,
            "<div class=\"alert alert-success\"><h3 style=\"margin-bottom:var(--space-2);\">âœ“ Thank You for Voting!</h3>"
            "<p>Your vote has been recorded successfully. You cannot change your vote once submitted.</p></div>",
            sizeof(alerts)-strlen(alerts)-1);
    }
    if (strcmp(status,"active")!=0) {
        char tmp[256];
        snprintf(tmp,256,
            "<div class=\"alert alert-warning\"><h3 style=\"margin-bottom:var(--space-2);\">âš ï¸ Election Not Active</h3>"
            "<p>This election is currently %s. Voting is only available during the active period.</p></div>",status);
        strncat(alerts,tmp,sizeof(alerts)-strlen(alerts)-1);
    }
    if (!eligible) {
        strncat(alerts,
            "<div class=\"alert alert-warning\"><h3 style=\"margin-bottom:var(--space-2);\">âš ï¸ Not Eligible to Vote</h3>"
            "<p>You are not on the eligible voters list for this election.</p></div>",
            sizeof(alerts)-strlen(alerts)-1);
    }

    /* Candidates */
    const char *csql =
        "SELECT candidate_id, name, description, image_path FROM candidates WHERE election_id=?;";
    sqlite3_prepare_v2(db_get(), csql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, eid);

    char candidates_html[32768]="";
    int can_vote = (strcmp(status,"active")==0 && !has_voted && eligible);
    if (can_vote) {
        char form_open[256];
        snprintf(form_open,256,
            "<form method=\"POST\" action=\"/vote\" id=\"voteForm\">"
            "<input type=\"hidden\" name=\"election_id\" value=\"%d\">"
            "<div class=\"grid grid-3\">", eid);
        strncpy(candidates_html, form_open, sizeof(candidates_html));
    } else {
        strncpy(candidates_html, "<div class=\"grid grid-3\">", sizeof(candidates_html));
    }

    int cand_count=0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int cid = sqlite3_column_int(stmt, 0);
        const char *cn = (const char*)sqlite3_column_text(stmt, 1);
        const char *cd = (const char*)sqlite3_column_text(stmt, 2);
        const char *ci = (const char*)sqlite3_column_text(stmt, 3);
        char cname[128]="",cdesc[512]="";
        if(cn) strncpy(cname,cn,127); if(cd) strncpy(cdesc,cd,511);
        char img_tag[512];
        if (ci && strcmp(ci,"default-candidate.png")!=0)
            snprintf(img_tag,sizeof(img_tag),"<img src=\"/images/%s\" alt=\"%s\" class=\"candidate-image\">",ci,cname);
        else
            snprintf(img_tag,sizeof(img_tag),"<div class=\"candidate-image\" style=\"background:var(--gradient-primary);display:flex;align-items:center;justify-content:center;color:white;font-size:var(--text-3xl);font-weight:bold;\">%c</div>",cname[0]?cname[0]:'?');

        char card[2048];
        if (can_vote) {
            snprintf(card,sizeof(card),
                "<label class=\"candidate-card card\">"
                "<input type=\"radio\" name=\"candidate_id\" value=\"%d\" required>"
                "%s<h3 class=\"text-center mb-3\">%s</h3>"
                "<p class=\"text-sm text-gray text-center\">%s</p>"
                "</label>", cid, img_tag, cname, cdesc[0]?cdesc:"No description provided");
        } else {
            snprintf(card,sizeof(card),
                "<div class=\"card text-center\">"
                "%s<h3 class=\"mb-3\">%s</h3>"
                "<p class=\"text-sm text-gray\">%s</p>"
                "</div>", img_tag, cname, cdesc[0]?cdesc:"No description provided");
        }
        strncat(candidates_html, card, sizeof(candidates_html)-strlen(candidates_html)-1);
        cand_count++;
    }
    sqlite3_finalize(stmt);
    strncat(candidates_html, "</div>", sizeof(candidates_html)-strlen(candidates_html)-1);

    if (can_vote) {
        strncat(candidates_html,
            "<div class=\"text-center mt-8\">"
            "<button type=\"submit\" class=\"btn btn-success btn-lg\" "
            "onclick=\"return confirm('Are you sure you want to submit your vote? This action cannot be undone.')\">ðŸ—³ï¸ Submit Vote</button>"
            "</div></form>", sizeof(candidates_html)-strlen(candidates_html)-1);
    }

    char no_cand[256]="";
    if (cand_count==0)
        strncpy(no_cand,"<div class=\"card text-center\" style=\"padding:var(--space-12);\"><h3 class=\"mb-3\">No Candidates Yet</h3><p class=\"text-gray\">Candidates have not been added yet.</p></div>",sizeof(no_cand));

    char eid_str[16]; snprintf(eid_str,16,"%d",eid);
    char tv_str[16]; snprintf(tv_str,16,"%d",total_votes);
    char cc_str[16]; snprintf(cc_str,16,"%d",cand_count);

    const char *stat_col = strcmp(status,"active")==0?"success":(strcmp(status,"upcoming")==0?"warning":"secondary");
    char back_url[64]; snprintf(back_url,64,"%s", s.is_admin?"/admin/elections":"/dashboard");

    const char *k[] = {"ELECTION_TITLE","ELECTION_DESC","ELECTION_STATUS","STATUS_COLOR",
                        "START_DATE","END_DATE","TOTAL_VOTES","TOTAL_CANDIDATES",
                        "ALERTS","CANDIDATES_HTML","NO_CANDIDATES","BACK_URL"};
    const char *v[] = {title,desc,status,stat_col,sdate,edate,tv_str,cc_str,
                        alerts,candidates_html,no_cand,back_url};
    send_html_page(c, "election_details.html", k, v, 12);
}

void handle_cast_vote(struct mg_connection *c, struct mg_http_message *hm) {
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    char cid_s[16]="", eid_s[16]="";
    form_get(hm,"candidate_id",cid_s,sizeof(cid_s));
    form_get(hm,"election_id", eid_s,sizeof(eid_s));
    int cid=atoi(cid_s), eid=atoi(eid_s);

    if (!cid || !eid) {
        redirect_with_flash(c,"/dashboard","Invalid vote submission.","danger"); return;
    }

    db_update_election_statuses();

    /* Check election is active */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT status FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char estatus[20]="";
    if(sqlite3_step(stmt)==SQLITE_ROW){const char *t=(const char*)sqlite3_column_text(stmt,0);if(t)strncpy(estatus,t,19);}
    sqlite3_finalize(stmt);

    if (strcmp(estatus,"active")!=0) {
        redirect_with_flash(c,"/dashboard","This election is not currently active.","warning"); return;
    }
    if (!db_is_eligible(eid, s.user_id)) {
        redirect_with_flash(c,"/dashboard","You are not eligible to vote in this election.","danger"); return;
    }
    /* Verify candidate belongs to election */
    sqlite3_prepare_v2(db_get(),"SELECT election_id FROM candidates WHERE candidate_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,cid);
    int cand_eid=0;
    if(sqlite3_step(stmt)==SQLITE_ROW) cand_eid=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    if (cand_eid != eid) {
        redirect_with_flash(c,"/dashboard","Invalid candidate selection.","danger"); return;
    }

    struct mg_str *peer = mg_http_get_header(hm,"X-Real-IP");
    char ip[64]="127.0.0.1";
    if (peer) { int n=peer->len<63?peer->len:63; memcpy(ip,peer->buf,n); ip[n]=0; }

    if (db_cast_vote(s.user_id, cid, eid, ip)) {
        char loc[64]; snprintf(loc,64,"/election/%d",eid);
        redirect_with_flash(c,loc,"Your vote has been recorded successfully!","success");
    } else {
        char loc[64]; snprintf(loc,64,"/election/%d",eid);
        redirect_with_flash(c,loc,"You have already voted in this election.","warning");
    }
}

/* ================================================================
   CANDIDATE APPLICATION
   ================================================================ */

void handle_apply_get(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    /* Checks */
    if (!db_is_eligible(eid, s.user_id)) {
        redirect_with_flash(c,"/dashboard","You must be an eligible voter to apply.","danger"); return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT status FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char estatus[20]="";
    if(sqlite3_step(stmt)==SQLITE_ROW){const char *t=(const char*)sqlite3_column_text(stmt,0);if(t)strncpy(estatus,t,19);}
    sqlite3_finalize(stmt);
    if (strcmp(estatus,"upcoming")!=0) {
        redirect_with_flash(c,"/dashboard","Applications only accepted for upcoming elections.","warning"); return;
    }
    int existing = db_user_has_applied(s.user_id, eid);
    if (existing) {
        char loc[64]; snprintf(loc,64,"/applications/%d",existing);
        redirect_with_flash(c,loc,"You have already applied for this election.","info"); return;
    }

    /* Get election title */
    char etitle[256]="";
    sqlite3_prepare_v2(db_get(),"SELECT title FROM elections WHERE election_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    if(sqlite3_step(stmt)==SQLITE_ROW){const char *t=(const char*)sqlite3_column_text(stmt,0);if(t)strncpy(etitle,t,255);}
    sqlite3_finalize(stmt);

    char eid_str[16]; snprintf(eid_str,16,"%d",eid);
    char cnic_fmt[20]; format_cnic(s.user_cnic, cnic_fmt);
    const char *k[]={"ELECTION_TITLE","ELECTION_ID","USER_NAME","USER_CNIC_FMT"};
    const char *v[]={etitle,eid_str,s.user_name,cnic_fmt};
    send_html_page(c,"apply_candidate.html",k,v,4);
}

void handle_apply_post(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    char desc[2048]="";
    form_get(hm,"description",desc,sizeof(desc));
    if (strlen(desc) < 10) {
        char loc[64]; snprintf(loc,64,"/election/%d/apply",eid);
        redirect_with_flash(c,loc,"Please provide a description (at least 10 characters).","danger");
        return;
    }

    int out_id=0;
    if (db_submit_application(s.user_id, eid, desc, "default-candidate.png", &out_id)) {
        redirect_with_flash(c,"/dashboard","Your application has been submitted for admin review.","success");
    } else {
        char loc[64]; snprintf(loc,64,"/election/%d/apply",eid);
        redirect_with_flash(c,loc,"You have already applied or an error occurred.","warning");
    }
}

void handle_view_application(struct mg_connection *c, struct mg_http_message *hm, int app_id) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT a.user_id, a.election_id, a.description, a.status, a.applied_at, "
        "       a.rejection_reason, e.title "
        "FROM candidate_applications a "
        "JOIN elections e ON e.election_id = a.election_id "
        "WHERE a.application_id=?;";
    sqlite3_prepare_v2(db_get(),sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,app_id);
    if (sqlite3_step(stmt)!=SQLITE_ROW) {
        sqlite3_finalize(stmt);
        redirect_with_flash(c,"/dashboard","Application not found.","danger"); return;
    }
    int auid=sqlite3_column_int(stmt,0);
    if (auid != s.user_id && !s.is_admin) {
        sqlite3_finalize(stmt);
        redirect_with_flash(c,"/dashboard","Access denied.","danger"); return;
    }
    char adesc[2048]="",astatus[20]="",adate[32]="",areason[512]="",etitle[256]="";
    const char *td=(const char*)sqlite3_column_text(stmt,2);
    const char *ts=(const char*)sqlite3_column_text(stmt,3);
    const char *ta=(const char*)sqlite3_column_text(stmt,4);
    const char *tr=(const char*)sqlite3_column_text(stmt,5);
    const char *te=(const char*)sqlite3_column_text(stmt,6);
    if(td) strncpy(adesc,td,2047); if(ts) strncpy(astatus,ts,19);
    if(ta) strncpy(adate,ta,31);  if(tr) strncpy(areason,tr,511);
    if(te) strncpy(etitle,te,255);
    sqlite3_finalize(stmt);

    const char *sc=strcmp(astatus,"approved")==0?"success":(strcmp(astatus,"pending")==0?"warning":"danger");
    char reason_html[640]="";
    if (areason[0]) snprintf(reason_html,sizeof(reason_html),
        "<div class=\"alert alert-danger\"><strong>Rejection Reason:</strong> %s</div>",areason);

    const char *k[]={"ELECTION_TITLE","APP_STATUS","STATUS_COLOR","APP_DESC","APP_DATE","REASON_HTML"};
    const char *v[]={etitle,astatus,sc,adesc,adate,reason_html};
    send_html_page(c,"application_status.html",k,v,6);
}

/* ================================================================
   PUBLIC RESULTS
   ================================================================ */

void handle_public_results(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid) { redirect_with_flash(c,"/login","Please log in.","warning"); return; }

    db_update_election_statuses();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT title,description,status FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    if(sqlite3_step(stmt)!=SQLITE_ROW){sqlite3_finalize(stmt);redirect_with_flash(c,"/dashboard","Election not found.","danger");return;}
    char etitle[256]="",edesc[1024]="",estatus[20]="";
    const char *t=(const char*)sqlite3_column_text(stmt,0);
    const char *d=(const char*)sqlite3_column_text(stmt,1);
    const char *st=(const char*)sqlite3_column_text(stmt,2);
    if(t)strncpy(etitle,t,255);if(d)strncpy(edesc,d,1023);if(st)strncpy(estatus,st,19);
    sqlite3_finalize(stmt);

    if (strcmp(estatus,"closed")!=0 && !s.is_admin) {
        redirect_with_flash(c,"/dashboard","Results are only available after the election closes.","warning");
        return;
    }

    int total_votes = db_get_total_votes_in_election(eid);

    /* Get results sorted by votes */
    const char *rsql =
        "SELECT c.candidate_id, c.name, c.description, "
        "COUNT(v.vote_id) as vote_count "
        "FROM candidates c "
        "LEFT JOIN votes v ON v.candidate_id=c.candidate_id "
        "WHERE c.election_id=? "
        "GROUP BY c.candidate_id ORDER BY vote_count DESC;";
    sqlite3_prepare_v2(db_get(),rsql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);

    char winner_html[512]="";
    char results_rows[16384]="";
    int idx=0;

    while(sqlite3_step(stmt)==SQLITE_ROW) {
        const char *cn=(const char*)sqlite3_column_text(stmt,1);
        const char *cd=(const char*)sqlite3_column_text(stmt,2);
        int vc=sqlite3_column_int(stmt,3);
        char cname[128]="",cdesc[512]="";
        if(cn)strncpy(cname,cn,127);if(cd)strncpy(cdesc,cd,511);
        double pct = total_votes>0 ? (vc*100.0/total_votes) : 0.0;

        if(idx==0 && total_votes>0) {
            snprintf(winner_html,sizeof(winner_html),
                "<div class=\"card mb-8\" style=\"background:var(--gradient-primary);color:white;padding:var(--space-10);text-align:center;\">"
                "<div style=\"font-size:var(--text-5xl);margin-bottom:var(--space-4);\">ðŸŽ‰</div>"
                "<h2 class=\"mb-4\" style=\"color:white;\">Winner: %s</h2>"
                "<p style=\"font-size:var(--text-2xl);opacity:0.9;\">%d votes (%.1f%%)</p>"
                "</div>",cname,vc,pct);
        }

        char row[1024];
        snprintf(row,sizeof(row),
            "<div>"
            "<div style=\"display:flex;justify-content:space-between;align-items:center;margin-bottom:var(--space-3);\">"
            "<div style=\"display:flex;align-items:center;gap:var(--space-4);\">"
            "<div style=\"width:50px;height:50px;background:var(--gradient-primary);border-radius:50%;display:flex;align-items:center;justify-content:center;color:white;font-weight:bold;font-size:var(--text-xl);\">#%d</div>"
            "<div><h3 class=\"mb-1\">%s</h3><p class=\"text-sm text-gray\">%s</p></div>"
            "</div>"
            "<div class=\"text-right\"><p class=\"fw-bold\" style=\"font-size:var(--text-2xl);color:var(--primary-600);\">%d</p><p class=\"text-sm text-gray\">votes</p></div>"
            "</div>"
            "<div class=\"progress\"><div class=\"progress-bar\" data-width=\"%.1f\" style=\"width:%.1f%%;\">%.1f%%</div></div>"
            "</div>",
            idx+1,cname,cdesc[0]?cdesc:"No description",vc,pct,pct,pct);
        strncat(results_rows,row,sizeof(results_rows)-strlen(results_rows)-1);
        idx++;
    }
    sqlite3_finalize(stmt);

    char tv_str[16]; snprintf(tv_str,16,"%d",total_votes);
    char tc_str[16]; snprintf(tc_str,16,"%d",idx);
    char back_url[64]; snprintf(back_url,64,"%s",s.is_admin?"/admin/elections":"/dashboard");
    const char *stat_col=strcmp(estatus,"active")==0?"success":(strcmp(estatus,"upcoming")==0?"warning":"secondary");

    char no_results[256]="";
    if(!idx) strncpy(no_results,"<div class=\"card text-center\" style=\"padding:var(--space-12);\"><h3 class=\"mb-3\">No Candidates</h3><p class=\"text-gray\">This election has no candidates.</p></div>",sizeof(no_results));

    const char *k[]={"ELECTION_TITLE","ELECTION_DESC","ELECTION_STATUS","STATUS_COLOR",
                      "TOTAL_VOTES","TOTAL_CANDIDATES","WINNER_HTML","RESULTS_ROWS","NO_RESULTS","BACK_URL"};
    const char *v[]={etitle,edesc,estatus,stat_col,tv_str,tc_str,winner_html,results_rows,no_results,back_url};
    send_html_page(c,"results.html",k,v,10);
}

/* ================================================================
   ADMIN DASHBOARD
   ================================================================ */

void handle_admin_dashboard(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid){redirect_with_flash(c,"/login","Please log in.","warning");return;}
    if(!s.is_admin){redirect_with_flash(c,"/dashboard","Access denied.","danger");return;}

    db_update_election_statuses();

    /* Stats */
    sqlite3_stmt *stmt;
    int total_voters=0,total_elections=0,total_votes=0,active_elections=0,pending_apps=0;
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM users WHERE is_admin=0 AND is_deleted=0;",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) total_voters=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM elections WHERE is_deleted=0;",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) total_elections=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM votes;",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) total_votes=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM elections WHERE status='active' AND is_deleted=0;",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) active_elections=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM candidate_applications WHERE status='pending';",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) pending_apps=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);

    /* Recent elections table */
    const char *esql =
        "SELECT election_id, title, status, start_date, "
        "(SELECT COUNT(*) FROM election_voters WHERE election_id=e.election_id) "
        "FROM elections e WHERE is_deleted=0 ORDER BY created_at DESC LIMIT 5;";
    sqlite3_prepare_v2(db_get(),esql,-1,&stmt,NULL);
    char recent_rows[8192]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int rid=sqlite3_column_int(stmt,0);
        const char *rt=(const char*)sqlite3_column_text(stmt,1);
        const char *rs=(const char*)sqlite3_column_text(stmt,2);
        const char *rsd=(const char*)sqlite3_column_text(stmt,3);
        int rv=sqlite3_column_int(stmt,4);
        char row[512];
        const char *sc=strcmp(rs,"active")==0?"success":"secondary";
        snprintf(row,512,
            "<tr><td>%s</td><td><span class=\"badge badge-%s\">%s</span></td>"
            "<td>%s</td><td>%d voters</td>"
            "<td><a href=\"/admin/elections/%d/applications\" class=\"btn btn-sm btn-outline\">View</a></td></tr>",
            rt?rt:"",sc,rs?rs:"",rsd?rsd:"",rv,rid);
        strncat(recent_rows,row,sizeof(recent_rows)-strlen(recent_rows)-1);
    }
    sqlite3_finalize(stmt);

    /* Pending applications */
    const char *asql =
        "SELECT a.application_id, u.name, u.cnic, e.title, a.applied_at "
        "FROM candidate_applications a "
        "JOIN users u ON u.user_id=a.user_id "
        "JOIN elections e ON e.election_id=a.election_id "
        "WHERE a.status='pending' ORDER BY a.applied_at DESC LIMIT 10;";
    sqlite3_prepare_v2(db_get(),asql,-1,&stmt,NULL);
    char pending_rows[8192]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int aid=sqlite3_column_int(stmt,0);
        const char *an=(const char*)sqlite3_column_text(stmt,1);
        const char *ac=(const char*)sqlite3_column_text(stmt,2);
        const char *ae=(const char*)sqlite3_column_text(stmt,3);
        const char *aat=(const char*)sqlite3_column_text(stmt,4);
        char cnic_fmt[20]=""; if(ac) format_cnic(ac,cnic_fmt);
        char row[1024];
        snprintf(row,sizeof(row),
            "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td><form method=\"POST\" action=\"/admin/applications/%d/approve\" style=\"display:inline;\">"
            "<button type=\"submit\" class=\"btn btn-sm btn-success\">Approve</button></form>"
            "<button type=\"button\" class=\"btn btn-sm btn-danger\" "
            "onclick=\"rejectApplication(%d)\">Reject</button></td></tr>",
            an?an:"",cnic_fmt,ae?ae:"",aat?aat:"",aid,aid);
        strncat(pending_rows,row,sizeof(pending_rows)-strlen(pending_rows)-1);
    }
    sqlite3_finalize(stmt);

    /* Pending password reset requests count */
    int pending_resets=0;
    sqlite3_prepare_v2(db_get(),"SELECT COUNT(*) FROM password_reset_requests WHERE status='pending';",-1,&stmt,NULL);
    if(sqlite3_step(stmt)==SQLITE_ROW) pending_resets=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);

    char tv_s[16],te_s[16],ta_s[16],tae_s[16],tp_s[16],tr_s[16];
    snprintf(tv_s,16,"%d",total_voters);
    snprintf(te_s,16,"%d",total_elections);
    snprintf(ta_s,16,"%d",total_votes);
    snprintf(tae_s,16,"%d",active_elections);
    snprintf(tp_s,16,"%d",pending_apps);
    snprintf(tr_s,16,"%d",pending_resets);

    /* Reset card styling â€” red border + color when there are pending requests */
    const char *reset_color  = pending_resets > 0 ? "#dc3545" : "inherit";
    const char *reset_border = pending_resets > 0 ? "2px solid rgba(220,53,69,0.5)" : "";

    char pa_section[9000]="";
    if(pending_rows[0]) {
        snprintf(pa_section,sizeof(pa_section),
            "<div class=\"card-glass p-6\">"
            "<h2 class=\"h3 mb-4\">Pending Applications</h2>"
            "<div class=\"table-responsive\"><table class=\"table\">"
            "<thead><tr><th>Applicant</th><th>CNIC</th><th>Election</th><th>Applied At</th><th>Actions</th></tr></thead>"
            "<tbody>%s</tbody></table></div></div>",
            pending_rows);
    }

    const char *k[]={"TOTAL_VOTERS","TOTAL_ELECTIONS","TOTAL_VOTES","ACTIVE_ELECTIONS",
                      "PENDING_APPS","RECENT_ROWS","PENDING_APPS_STR","PENDING_SECTION",
                      "RESET_REQUESTS","RESET_COLOR","RESET_BORDER","USER_NAME"};
    const char *v[]={tv_s,te_s,ta_s,tae_s,tp_s,recent_rows,tp_s,pa_section,
                     tr_s,reset_color,reset_border,s.user_name};
    send_html_page(c,"admin_dashboard.html",k,v,12);
}

/* ================================================================
   ADMIN VOTER MANAGEMENT
   ================================================================ */

void handle_admin_voters(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    char search[128]="";
    struct mg_str qs = hm->query;
    if (qs.len) {
        char tmp[qs.len+1]; memcpy(tmp,qs.buf,qs.len); tmp[qs.len]=0;
        mg_url_decode(tmp, strlen(tmp), search, sizeof(search), 1);
        /* extract search=value */
        char *p = strstr(search,"search=");
        if(p) {
            memmove(search,p+7,strlen(p+7)+1);
            char *amp=strchr(search,'&'); if(amp)*amp=0;
            for(int i=0;search[i];i++) if(search[i]=='+') search[i]=' ';
            mg_url_decode(search,strlen(search),search,sizeof(search),1);
        } else { search[0]=0; }
    }

    /* Check ?filter=resets to show only voters with pending reset requests */
    int filter_resets = 0;
    if(qs.len) {
        char qsbuf[256]; int qsn=qs.len<255?qs.len:255;
        memcpy(qsbuf,qs.buf,qsn); qsbuf[qsn]=0;
        if(strstr(qsbuf,"filter=resets")) filter_resets=1;
    }

    const char *vsql;
    char vsql_like[512];
    sqlite3_stmt *stmt;

    if(filter_resets) {
        vsql="SELECT u.user_id,u.name,u.cnic,u.email,u.is_deleted,u.created_at "
             "FROM users u "
             "INNER JOIN password_reset_requests r ON r.user_id=u.user_id AND r.status='pending' "
             "WHERE u.is_admin=0 ORDER BY u.name ASC;";
        sqlite3_prepare_v2(db_get(),vsql,-1,&stmt,NULL);
    } else if (search[0]) {
        snprintf(vsql_like,sizeof(vsql_like),
            "SELECT user_id,name,cnic,email,is_deleted,created_at FROM users WHERE is_admin=0 "
            "AND (name LIKE '%%%s%%' OR cnic LIKE '%%%s%%' OR email LIKE '%%%s%%') "
            "ORDER BY is_deleted ASC, created_at DESC;",search,search,search);
        sqlite3_prepare_v2(db_get(),vsql_like,-1,&stmt,NULL);
    } else {
        vsql="SELECT user_id,name,cnic,email,is_deleted,created_at FROM users WHERE is_admin=0 "
             "ORDER BY is_deleted ASC, created_at DESC;";
        sqlite3_prepare_v2(db_get(),vsql,-1,&stmt,NULL);
    }

    /* Build a lookup set of user_ids with pending reset requests */
    char reset_uids[4096]=",";
    sqlite3_stmt *rstmt;
    sqlite3_prepare_v2(db_get(),
        "SELECT user_id FROM password_reset_requests WHERE status='pending';",
        -1,&rstmt,NULL);
    while(sqlite3_step(rstmt)==SQLITE_ROW){
        char tmp[16]; snprintf(tmp,16,"%d,",sqlite3_column_int(rstmt,0));
        strncat(reset_uids,tmp,sizeof(reset_uids)-strlen(reset_uids)-1);
    }
    sqlite3_finalize(rstmt);

    char rows[32768]="";
    int count=0;
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int uid=sqlite3_column_int(stmt,0);
        const char *nm=(const char*)sqlite3_column_text(stmt,1);
        const char *cn=(const char*)sqlite3_column_text(stmt,2);
        const char *em=(const char*)sqlite3_column_text(stmt,3);
        int banned=sqlite3_column_int(stmt,4);
        const char *dt=(const char*)sqlite3_column_text(stmt,5);
        char cnic_fmt[20]=""; if(cn) format_cnic(cn,cnic_fmt);
        char date_str[12]=""; if(dt&&strlen(dt)>=10){strncpy(date_str,dt,10);}

        /* Check if this voter has a pending reset request */
        char uid_needle[16]; snprintf(uid_needle,16,",%d,",uid);
        int has_reset = (strstr(reset_uids, uid_needle) != NULL);

        /* Row style: red background for pending reset, yellow for banned */
        const char *row_style;
        if(has_reset && !banned)
            row_style=" style=\"background:rgba(220,53,69,0.12);border-left:4px solid #dc3545;\"";
        else if(banned)
            row_style=" style=\"opacity:0.6;background-color:#fff3cd;\"";
        else
            row_style="";

        char reset_badge[100]="";
        if(has_reset)
            snprintf(reset_badge,sizeof(reset_badge),
                " <span class=\"badge badge-danger\" title=\"Password reset requested\">ðŸ”‘ Reset</span>");

        char action_btns[1024];
        if(!banned)
            snprintf(action_btns,sizeof(action_btns),
                "<a href=\"/admin/voters/%d/edit\" class=\"btn btn-sm btn-info\">âœï¸ Edit</a>"
                "<form method=\"POST\" action=\"/admin/voters/%d/ban\" style=\"display:inline;\""
                " onsubmit=\"return confirm('Ban this voter?');\"><button type=\"submit\" class=\"btn btn-sm btn-warning\">ðŸš« Ban</button></form>"
                "<form method=\"POST\" action=\"/admin/voters/%d/delete\" style=\"display:inline;\""
                " onsubmit=\"return confirm('Permanently delete this voter and ALL their data?\\nThis cannot be undone!');\"><button type=\"submit\" class=\"btn btn-sm btn-danger\">ðŸ—‘ï¸</button></form>",
                uid,uid,uid);
        else
            snprintf(action_btns,sizeof(action_btns),
                "<form method=\"POST\" action=\"/admin/voters/%d/unban\" style=\"display:inline;\"><button type=\"submit\" class=\"btn btn-sm btn-success\">âœ… Unban</button></form>"
                "<form method=\"POST\" action=\"/admin/voters/%d/delete\" style=\"display:inline;\""
                " onsubmit=\"return confirm('Permanently delete this voter and ALL their data?\\nThis cannot be undone!');\"><button type=\"submit\" class=\"btn btn-sm btn-danger\">ðŸ—‘ï¸</button></form>",
                uid,uid);

        char row[1200];
        snprintf(row,sizeof(row),
            "<tr%s><td>%s%s</td><td>%s</td><td>%s</td>"
            "<td><span class=\"badge badge-%s\">%s</span></td>"
            "<td>%s</td><td>%s</td></tr>",
            row_style,
            nm?nm:"", reset_badge, cnic_fmt, em?em:"",
            banned?"danger":"success", banned?"BANNED":"Active",
            date_str, action_btns);
        strncat(rows,row,sizeof(rows)-strlen(rows)-1);
        count++;
    }
    sqlite3_finalize(stmt);

    char count_str[16]; snprintf(count_str,16,"%d",count);
    char no_voters[256]="";
    if(!count){
        if(filter_resets)
            snprintf(no_voters,sizeof(no_voters),
                "<p class=\"text-gray\">No voters with pending password reset requests.</p>");
        else
            snprintf(no_voters,sizeof(no_voters),
                "<p class=\"text-gray\">No voters found%s%s%s.</p>",
                search[0]?" matching \"":"",search,search[0]?"\"":"");
    }

    /* Page heading changes when filter active */
    const char *heading = filter_resets ?
        "ðŸ”‘ Voters with Password Reset Requests" : "Manage Voters";

    const char *k[]={"SEARCH_QUERY","VOTER_COUNT","VOTER_ROWS","NO_VOTERS_MSG","USER_NAME","PAGE_HEADING"};
    const char *v[]={search,count_str,rows,no_voters,s.user_name,heading};
    send_html_page(c,"admin_manage_voters.html",k,v,6);
}

void handle_admin_add_voter_get(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    const char *k[]={"NAME_VAL","CNIC_VAL","EMAIL_VAL","USER_NAME"};
    const char *v[]={"","","",s.user_name};
    send_html_page(c,"admin_add_voter.html",k,v,4);
}

void handle_admin_add_voter_post(struct mg_connection *c, struct mg_http_message *hm) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char name[128]="",cnic[32]="",email[128]="",password[128]="";
    form_get(hm,"name",name,sizeof(name));
    form_get(hm,"cnic",cnic,sizeof(cnic));
    form_get(hm,"email",email,sizeof(email));
    form_get(hm,"password",password,sizeof(password));

    clean_cnic(cnic);
    if(strlen(name)<2||strlen(cnic)!=13||strlen(email)<5||strlen(password)<8){
        redirect_with_flash(c,"/admin/voters/add",
            "Validation failed. Check name (2+ chars), CNIC (13 digits), email, password (8+ chars).","danger");
        return;
    }
    char hashed[65]; sha256_string(password,hashed);
    if(db_create_user(cnic,name,email,hashed,0)){
        export_all_data_xls(); redirect_with_flash(c,"/admin/voters","Voter added successfully.","success");
    } else {
        redirect_with_flash(c,"/admin/voters/add","CNIC or email already exists.","danger");
    }
}

void handle_admin_edit_voter_get(struct mg_connection *c, struct mg_http_message *hm, int uid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char name[128]="",cnic[20]="",email[120]="",created_at[32]="";
    int is_adm=0,is_del=0;
    if(!db_get_user(uid,name,cnic,email,&is_adm,&is_del,created_at)||is_adm){
        redirect_with_flash(c,"/admin/voters","Voter not found.","danger");return;
    }
    char cnic_fmt[20]; format_cnic(cnic,cnic_fmt);
    char uid_s[16]; snprintf(uid_s,16,"%d",uid);
    const char *k[]={"VOTER_ID","VOTER_NAME","VOTER_CNIC","VOTER_EMAIL","USER_NAME"};
    const char *v[]={uid_s,name,cnic_fmt,email,s.user_name};
    send_html_page(c,"admin_edit_voter.html",k,v,5);
}

void handle_admin_edit_voter_post(struct mg_connection *c, struct mg_http_message *hm, int uid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    /* CNIC is NOT accepted from form â€” it is read-only. Email can be updated. */
    char name[128]="", email[128]="", new_password[128]="";
    form_get(hm,"name",name,sizeof(name));
    form_get(hm,"email",email,sizeof(email));
    form_get(hm,"new_password",new_password,sizeof(new_password));

    if(strlen(name)<2 || strlen(email)<5 || !strchr(email,'@')){
        char loc[64]; snprintf(loc,64,"/admin/voters/%d/edit",uid);
        redirect_with_flash(c,loc,"Name (2+ chars) and valid email required.","danger");
        return;
    }
    db_update_user(uid, name, email);
    if(new_password[0] && strlen(new_password)>=6){
        char hashed[65]; sha256_string(new_password,hashed);
        db_update_password(uid,hashed);
        /* Auto-resolve any pending password reset request for this voter */
        const char *del_reset =
            "UPDATE password_reset_requests SET status='resolved', "
            "resolved_at=datetime('now') WHERE user_id=? AND status='pending';";
        sqlite3_stmt *rs;
        if(sqlite3_prepare_v2(db_get(),del_reset,-1,&rs,NULL)==SQLITE_OK){
            sqlite3_bind_int(rs,1,uid);
            sqlite3_step(rs);
            sqlite3_finalize(rs);
        }
    }
    export_all_data_xls(); redirect_with_flash(c,"/admin/voters","Voter updated successfully.","success");
}

void handle_admin_ban_voter(struct mg_connection *c, struct mg_http_message *hm, int uid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){send_redirect(c,"/login");return;}
    if(db_ban_user(uid)){export_all_data_xls();redirect_with_flash(c,"/admin/voters","Voter banned.","success");}
    else                  redirect_with_flash(c,"/admin/voters","Could not ban voter.","danger");
}

void handle_admin_unban_voter(struct mg_connection *c, struct mg_http_message *hm, int uid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){send_redirect(c,"/login");return;}
    if(db_unban_user(uid)){export_all_data_xls();redirect_with_flash(c,"/admin/voters","Voter unbanned.","success");}
    else                    redirect_with_flash(c,"/admin/voters","Could not unban voter.","danger");
}

void handle_admin_delete_voter_permanent(struct mg_connection *c, struct mg_http_message *hm, int uid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){send_redirect(c,"/login");return;}
    /* Safety: cannot delete admins */
    char nm[128]=""; int ia=0,idel=0; char cn[32]="",em[128]="",cat[32]="";
    if(!db_get_user(uid,nm,cn,em,&ia,&idel,cat)||ia){
        redirect_with_flash(c,"/admin/voters","Voter not found or cannot delete admin.","danger");
        return;
    }
    /* Hard-delete all related data then the user */
    sqlite3 *db = db_get();
    sqlite3_exec(db,"BEGIN;",0,0,NULL);
    char sql[256];
    snprintf(sql,sizeof(sql),"DELETE FROM votes WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM candidate_applications WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM candidates WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM election_voters WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM sessions WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM password_reset_requests WHERE user_id=%d;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    snprintf(sql,sizeof(sql),"DELETE FROM users WHERE user_id=%d AND is_admin=0;",uid);
    sqlite3_exec(db,sql,0,0,NULL);
    sqlite3_exec(db,"COMMIT;",0,0,NULL);
    char msg[200];
    snprintf(msg,sizeof(msg),"Voter '%s' permanently deleted from the system.",nm);
    redirect_with_flash(c,"/admin/voters",msg,"success");
}

/* ================================================================
   ADMIN ELECTIONS
   ================================================================ */

void handle_admin_elections(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    db_update_election_statuses();

    sqlite3_stmt *stmt;
    const char *esql=
        "SELECT e.election_id, e.title, e.status, e.start_date, e.end_date,"
        "(SELECT COUNT(*) FROM election_voters WHERE election_id=e.election_id),"
        "(SELECT COUNT(*) FROM candidates WHERE election_id=e.election_id),"
        "(SELECT COUNT(*) FROM votes WHERE election_id=e.election_id) "
        "FROM elections e WHERE is_deleted=0 ORDER BY created_at DESC;";
    sqlite3_prepare_v2(db_get(),esql,-1,&stmt,NULL);
    char rows[32768]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int eid=sqlite3_column_int(stmt,0);
        const char *et=(const char*)sqlite3_column_text(stmt,1);
        const char *es=(const char*)sqlite3_column_text(stmt,2);
        const char *sd=(const char*)sqlite3_column_text(stmt,3);
        const char *ed=(const char*)sqlite3_column_text(stmt,4);
        int ev=sqlite3_column_int(stmt,5);
        int ec=sqlite3_column_int(stmt,6);
        int evo=sqlite3_column_int(stmt,7);
        const char *sc=strcmp(es,"active")==0?"success":(strcmp(es,"upcoming")==0?"warning":"secondary");
        int is_upcoming=(strcmp(es,"upcoming")==0);

        char actions[1024]="";
        if(is_upcoming){
            char tmp[512];
            snprintf(tmp,sizeof(tmp),
                "<a href=\"/admin/elections/%d/voters\" class=\"btn btn-sm btn-info\">Voters</a>"
                "<a href=\"/admin/elections/%d/candidates\" class=\"btn btn-sm btn-info\">Candidates</a>"
                "<a href=\"/admin/elections/%d/edit\" class=\"btn btn-sm btn-warning\">Edit</a>",eid,eid,eid);
            strncpy(actions,tmp,sizeof(actions));
        }
        char tmp2[256];
        snprintf(tmp2,sizeof(tmp2),"<a href=\"/admin/elections/%d/applications\" class=\"btn btn-sm btn-secondary\">Applications</a>",eid);
        strncat(actions,tmp2,sizeof(actions)-strlen(actions)-1);
        if(strcmp(es,"closed")==0||evo>0){
            snprintf(tmp2,sizeof(tmp2),"<a href=\"/admin/results/%d\" class=\"btn btn-sm btn-success\">Results</a>",eid);
            strncat(actions,tmp2,sizeof(actions)-strlen(actions)-1);
        }
        if(is_upcoming&&evo==0){
            snprintf(tmp2,sizeof(tmp2),
                "<form method=\"POST\" action=\"/admin/elections/%d/delete\" style=\"display:inline;\" "
                "onsubmit=\"return confirm('Archive this election?');\">"
                "<button type=\"submit\" class=\"btn btn-sm btn-danger\">Archive</button></form>",eid);
            strncat(actions,tmp2,sizeof(actions)-strlen(actions)-1);
        }

        char row[2048];
        snprintf(row,sizeof(row),
            "<tr><td>%s</td><td><span class=\"badge badge-%s\">%s</span></td>"
            "<td><small>%s<br>to %s</small></td><td>%d</td><td>%d</td><td>%d</td>"
            "<td><div class=\"btn-group-vertical btn-group-sm\">%s</div></td></tr>",
            et?et:"",sc,es?es:"",sd?sd:"",ed?ed:"",ev,ec,evo,actions);
        strncat(rows,row,sizeof(rows)-strlen(rows)-1);
    }
    sqlite3_finalize(stmt);

    char no_msg[64]="";
    if(!rows[0]) strcpy(no_msg,"<p class=\"text-gray\">No elections created yet.</p>");

    const char *k[]={"ELECTION_ROWS","NO_ELECTIONS_MSG","USER_NAME"};
    const char *v[]={rows,no_msg,s.user_name};
    send_html_page(c,"admin_elections.html",k,v,3);
}

void handle_admin_create_election_get(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),
        "SELECT user_id,name,cnic FROM users WHERE is_admin=0 AND is_deleted=0 ORDER BY name;",-1,&stmt,NULL);
    char voter_checkboxes[16384]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int vid=sqlite3_column_int(stmt,0);
        const char *vn=(const char*)sqlite3_column_text(stmt,1);
        const char *vc=(const char*)sqlite3_column_text(stmt,2);
        char cnic_fmt[20]=""; if(vc) format_cnic(vc,cnic_fmt);
        char row[512];
        snprintf(row,sizeof(row),
            "<div class=\"col-md-6\"><div class=\"form-check\">"
            "<input class=\"form-check-input voter-checkbox\" type=\"checkbox\" "
            "name=\"eligible_voters[]\" value=\"%d\" id=\"voter-%d\">"
            "<label class=\"form-check-label\" for=\"voter-%d\">%s <span class=\"text-gray\">(%s)</span></label>"
            "</div></div>",vid,vid,vid,vn?vn:"",cnic_fmt);
        strncat(voter_checkboxes,row,sizeof(voter_checkboxes)-strlen(voter_checkboxes)-1);
    }
    sqlite3_finalize(stmt);
    if(!voter_checkboxes[0])
        strncpy(voter_checkboxes,"<p class=\"text-gray\">No voters registered yet. <a href=\"/admin/voters/add\">Add voters first</a>.</p>",sizeof(voter_checkboxes));

    const char *k[]={"VOTER_CHECKBOXES","USER_NAME"};
    const char *v[]={voter_checkboxes,s.user_name};
    send_html_page(c,"admin_create_election.html",k,v,2);
}

void handle_admin_create_election_post(struct mg_connection *c, struct mg_http_message *hm) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    char title[256]="",desc[2048]="",start_date[32]="",end_date[32]="";
    form_get(hm,"title",title,sizeof(title));
    form_get(hm,"description",desc,sizeof(desc));
    form_get(hm,"start_date",start_date,sizeof(start_date));
    form_get(hm,"end_date",end_date,sizeof(end_date));

    /* Parse voter IDs from body manually */
    char body_copy[hm->body.len+1];
    memcpy(body_copy,hm->body.buf,hm->body.len);
    body_copy[hm->body.len]=0;

    int voter_ids[1024]; int voter_count=0;
    char *ptr=body_copy;
    while((ptr=strstr(ptr,"eligible_voters%5B%5D="))!=NULL||
          (ptr=strstr(ptr,"eligible_voters[]="))!=NULL) {
        /* try both encoded and raw */
        char *p2=strstr(body_copy,"eligible_voters%5B%5D=");
        if(!p2) p2=strstr(body_copy,"eligible_voters[]=");
        if(!p2) break;

        /* Scan all occurrences */
        const char *tag1="eligible_voters%5B%5D=";
        const char *tag2="eligible_voters[]=";
        char *q=body_copy;
        voter_count=0;
        while(voter_count<1024){
            char *f1=strstr(q,tag1);
            char *f2=strstr(q,tag2);
            char *f=NULL;
            int tlen=0;
            if(f1&&f2) {f=f1<f2?f1:f2; tlen=f==f1?strlen(tag1):strlen(tag2);}
            else if(f1){f=f1;tlen=strlen(tag1);}
            else if(f2){f=f2;tlen=strlen(tag2);}
            else break;
            q=f+tlen;
            voter_ids[voter_count++]=atoi(q);
        }
        break;
    }

    if(strlen(title)<3||voter_count==0||!start_date[0]||!end_date[0]){
        redirect_with_flash(c,"/admin/elections/create",
            "Title (3+ chars), dates, and at least one voter required.","danger");
        return;
    }
    /* Replace T with space for SQLite */
    for(int i=0;start_date[i];i++) if(start_date[i]=='T') start_date[i]=' ';
    for(int i=0;end_date[i];i++)   if(end_date[i]=='T')   end_date[i]=' ';

    /* Determine status */
    char status[16]="upcoming";

    int eid=0;
    if(db_create_election(title,desc,start_date,end_date,status,&eid)){
        db_set_election_voters(eid,voter_ids,voter_count);
        char msg[128]; snprintf(msg,128,"Election \"%s\" created successfully!",title);
        redirect_with_flash(c,"/admin/elections",msg,"success");
    } else {
        redirect_with_flash(c,"/admin/elections/create","An error occurred.","danger");
    }
}

void handle_admin_edit_election_get(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),
        "SELECT title,description,start_date,end_date FROM elections WHERE election_id=? AND is_deleted=0;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    if(sqlite3_step(stmt)!=SQLITE_ROW){
        sqlite3_finalize(stmt);
        redirect_with_flash(c,"/admin/elections","Election not found.","danger");return;
    }
    char et[256]="",ed[2048]="",esd[32]="",eed[32]="";
    const char *t=(const char*)sqlite3_column_text(stmt,0);
    const char *d=(const char*)sqlite3_column_text(stmt,1);
    const char *sd=(const char*)sqlite3_column_text(stmt,2);
    const char *endd=(const char*)sqlite3_column_text(stmt,3);
    if(t)strncpy(et,t,255);if(d)strncpy(ed,d,2047);
    if(sd)strncpy(esd,sd,31);if(endd)strncpy(eed,endd,31);
    sqlite3_finalize(stmt);
    /* Convert space to T for datetime-local */
    for(int i=0;esd[i];i++) if(esd[i]==' ') esd[i]='T';
    for(int i=0;eed[i];i++) if(eed[i]==' ') eed[i]='T';
    /* Trim seconds if present e.g. 2024-01-01T12:00:00 -> 2024-01-01T12:00 */
    if(strlen(esd)>16) esd[16]=0;
    if(strlen(eed)>16) eed[16]=0;

    char eid_s[16]; snprintf(eid_s,16,"%d",eid);
    const char *k[]={"ELECTION_ID","ELECTION_TITLE","ELECTION_DESC","START_DATE","END_DATE","USER_NAME"};
    const char *v[]={eid_s,et,ed,esd,eed,s.user_name};
    send_html_page(c,"admin_edit_election.html",k,v,6);
}

void handle_admin_edit_election_post(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char title[256]="",desc[2048]="",start_date[32]="",end_date[32]="";
    form_get(hm,"title",title,sizeof(title));
    form_get(hm,"description",desc,sizeof(desc));
    form_get(hm,"start_date",start_date,sizeof(start_date));
    form_get(hm,"end_date",end_date,sizeof(end_date));
    for(int i=0;start_date[i];i++) if(start_date[i]=='T') start_date[i]=' ';
    for(int i=0;end_date[i];i++)   if(end_date[i]=='T')   end_date[i]=' ';
    if(strlen(title)<3){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/edit",eid);
        redirect_with_flash(c,loc,"Title must be at least 3 characters.","danger");return;
    }
    if(db_update_election(eid,title,desc,start_date,end_date))
        redirect_with_flash(c,"/admin/elections","Election updated successfully.","success");
    else {
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/edit",eid);
        redirect_with_flash(c,loc,"Error updating election.","danger");
    }
}

void handle_admin_election_voters_get(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    /* Check election is upcoming */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT status,title FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char estatus[20]="",etitle[256]="";
    if(sqlite3_step(stmt)==SQLITE_ROW){
        const char *st=(const char*)sqlite3_column_text(stmt,0);
        const char *tt=(const char*)sqlite3_column_text(stmt,1);
        if(st)strncpy(estatus,st,19);if(tt)strncpy(etitle,tt,255);
    }
    sqlite3_finalize(stmt);
    if(strcmp(estatus,"upcoming")!=0){
        redirect_with_flash(c,"/admin/elections","Cannot modify voter list after election has started.","warning");return;
    }

    /* Get all non-deleted non-admin voters */
    sqlite3_prepare_v2(db_get(),
        "SELECT user_id,name,cnic FROM users WHERE is_admin=0 AND is_deleted=0 ORDER BY name;",
        -1,&stmt,NULL);
    char checkboxes[16384]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int vid=sqlite3_column_int(stmt,0);
        const char *vn=(const char*)sqlite3_column_text(stmt,1);
        const char *vc=(const char*)sqlite3_column_text(stmt,2);
        char cnic_fmt[20]=""; if(vc) format_cnic(vc,cnic_fmt);
        int checked=db_is_eligible(eid,vid);
        char row[512];
        snprintf(row,sizeof(row),
            "<div class=\"col-md-6\"><div class=\"form-check\">"
            "<input class=\"form-check-input voter-checkbox\" type=\"checkbox\" "
            "name=\"eligible_voters[]\" value=\"%d\" id=\"voter-%d\"%s>"
            "<label class=\"form-check-label\" for=\"voter-%d\">%s <span class=\"text-gray\">(%s)</span></label>"
            "</div></div>",vid,vid,checked?" checked":"",vid,vn?vn:"",cnic_fmt);
        strncat(checkboxes,row,sizeof(checkboxes)-strlen(checkboxes)-1);
    }
    sqlite3_finalize(stmt);

    char eid_s[16]; snprintf(eid_s,16,"%d",eid);
    const char *k[]={"ELECTION_ID","ELECTION_TITLE","VOTER_CHECKBOXES","USER_NAME"};
    const char *v[]={eid_s,etitle,checkboxes,s.user_name};
    send_html_page(c,"admin_manage_election_voters.html",k,v,4);
}

void handle_admin_election_voters_post(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    char body_copy[hm->body.len+1];
    memcpy(body_copy,hm->body.buf,hm->body.len);
    body_copy[hm->body.len]=0;

    int voter_ids[1024]; int voter_count=0;
    const char *tags[]={"eligible_voters%5B%5D=","eligible_voters[]="};
    for(int t=0;t<2;t++){
        char *q=body_copy;
        while(voter_count<1024){
            char *f=strstr(q,tags[t]);
            if(!f) break;
            q=f+strlen(tags[t]);
            voter_ids[voter_count++]=atoi(q);
        }
        if(voter_count) break;
    }

    if(voter_count==0){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/voters",eid);
        redirect_with_flash(c,loc,"Must select at least one eligible voter.","danger");return;
    }
    db_set_election_voters(eid,voter_ids,voter_count);
    redirect_with_flash(c,"/admin/elections","Eligible voters updated successfully.","success");
}

void handle_admin_delete_election(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    if(db_delete_election(eid,s.user_id))
        redirect_with_flash(c,"/admin/elections","Election archived.","success");
    else
        redirect_with_flash(c,"/admin/elections","Cannot archive (active or has votes).","danger");
}

/* ================================================================
   ADMIN APPLICATIONS
   ================================================================ */

void handle_admin_applications(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    char status_filter[20]="all";
    char qs_str[256]="";
    if(hm->query.len){
        int n=hm->query.len<255?hm->query.len:255;
        memcpy(qs_str,hm->query.buf,n); qs_str[n]=0;
        char *p=strstr(qs_str,"status="); if(p){strncpy(status_filter,p+7,19); char *amp=strchr(status_filter,'&');if(amp)*amp=0;}
    }

    char where[64]="";
    if(strcmp(status_filter,"pending")==0)  strncpy(where,"WHERE a.status='pending'",63);
    else if(strcmp(status_filter,"approved")==0) strncpy(where,"WHERE a.status='approved'",63);
    else if(strcmp(status_filter,"rejected")==0) strncpy(where,"WHERE a.status='rejected'",63);

    char sql[512];
    snprintf(sql,sizeof(sql),
        "SELECT a.application_id, u.name, u.cnic, e.title, a.status, a.applied_at "
        "FROM candidate_applications a "
        "JOIN users u ON u.user_id=a.user_id "
        "JOIN elections e ON e.election_id=a.election_id "
        "%s ORDER BY a.applied_at DESC;", where);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),sql,-1,&stmt,NULL);
    char rows[32768]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int aid=sqlite3_column_int(stmt,0); (void)aid;
        const char *an=(const char*)sqlite3_column_text(stmt,1);
        const char *ac=(const char*)sqlite3_column_text(stmt,2);
        const char *ae=(const char*)sqlite3_column_text(stmt,3);
        const char *as2=(const char*)sqlite3_column_text(stmt,4);
        const char *aat=(const char*)sqlite3_column_text(stmt,5);
        char cnic_fmt[20]=""; if(ac) format_cnic(ac,cnic_fmt);
        const char *sc=strcmp(as2?as2:"","approved")==0?"success":(strcmp(as2?as2:"","pending")==0?"warning":"danger");
        char row[1024];
        snprintf(row,sizeof(row),
            "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td><span class=\"badge badge-%s\">%s</span></td></tr>",
            an?an:"",cnic_fmt,ae?ae:"",aat?aat:"",sc,as2?as2:"");
        strncat(rows,row,sizeof(rows)-strlen(rows)-1);
    }
    sqlite3_finalize(stmt);

    char no_msg[64]=""; if(!rows[0]) strcpy(no_msg,"<p class=\"text-gray\">No applications found.</p>");
    const char *k[]={"APPLICATION_ROWS","NO_APPS_MSG","STATUS_FILTER","USER_NAME"};
    const char *v[]={rows,no_msg,status_filter,s.user_name};
    send_html_page(c,"admin_applications.html",k,v,4);
}

void handle_admin_election_applications(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    char etitle[256]="";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT title FROM elections WHERE election_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    if(sqlite3_step(stmt)==SQLITE_ROW){const char *t=(const char*)sqlite3_column_text(stmt,0);if(t)strncpy(etitle,t,255);}
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db_get(),
        "SELECT a.application_id, u.name, u.cnic, a.description, a.status, a.applied_at "
        "FROM candidate_applications a JOIN users u ON u.user_id=a.user_id "
        "WHERE a.election_id=? ORDER BY a.applied_at DESC;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char cards[32768]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int aid=sqlite3_column_int(stmt,0);
        const char *an=(const char*)sqlite3_column_text(stmt,1);
        const char *ac=(const char*)sqlite3_column_text(stmt,2);
        const char *ad=(const char*)sqlite3_column_text(stmt,3);
        const char *as2=(const char*)sqlite3_column_text(stmt,4);
        const char *aat=(const char*)sqlite3_column_text(stmt,5);
        char cnic_fmt[20]=""; if(ac) format_cnic(ac,cnic_fmt);
        const char *sc=strcmp(as2?as2:"","approved")==0?"success":(strcmp(as2?as2:"","pending")==0?"warning":"danger");
        char action_btns[512]="";
        if(strcmp(as2?as2:"","pending")==0){
            snprintf(action_btns,sizeof(action_btns),
                "<div class=\"mt-3\">"
                "<form method=\"POST\" action=\"/admin/applications/%d/approve\" style=\"display:inline;\">"
                "<button type=\"submit\" class=\"btn btn-success btn-sm\">Approve</button></form>"
                "<button class=\"btn btn-danger btn-sm\" onclick=\"rejectApp(%d)\">Reject</button>"
                "</div>",aid,aid);
        }
        char card[2048];
        snprintf(card,sizeof(card),
            "<div class=\"card-glass p-4 mb-3\">"
            "<div class=\"d-flex justify-content-between\">"
            "<div><h3 class=\"h5\">%s</h3>"
            "<p class=\"text-gray\">CNIC: %s</p>"
            "<p>%s</p><small class=\"text-gray\">Applied: %s</small></div>"
            "<div><span class=\"badge badge-%s\">%s</span></div>"
            "</div>%s</div>",
            an?an:"",cnic_fmt,ad?ad:"",aat?aat:"",sc,as2?as2:"",action_btns);
        strncat(cards,card,sizeof(cards)-strlen(cards)-1);
    }
    sqlite3_finalize(stmt);
    if(!cards[0]) strncpy(cards,"<p class=\"text-gray\">No applications yet.</p>",sizeof(cards));

    char eid_s[16]; snprintf(eid_s,16,"%d",eid);
    const char *k[]={"ELECTION_ID","ELECTION_TITLE","APPLICATION_CARDS","USER_NAME"};
    const char *v[]={eid_s,etitle,cards,s.user_name};
    send_html_page(c,"admin_election_applications.html",k,v,4);
}

void handle_admin_approve_application(struct mg_connection *c, struct mg_http_message *hm, int app_id) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    /* Get election_id for redirect */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT election_id FROM candidate_applications WHERE application_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,app_id);
    int eid=0;
    if(sqlite3_step(stmt)==SQLITE_ROW) eid=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);

    if(db_approve_application(app_id,s.user_id)){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/applications",eid);
        redirect_with_flash(c,loc,"Application approved.","success");
    } else {
        redirect_with_flash(c,"/admin/applications","Could not approve application.","danger");
    }
}

void handle_admin_reject_application(struct mg_connection *c, struct mg_http_message *hm, int app_id) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char reason[512]="";
    form_get(hm,"reason",reason,sizeof(reason));
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT election_id FROM candidate_applications WHERE application_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,app_id);
    int eid=0;
    if(sqlite3_step(stmt)==SQLITE_ROW) eid=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    db_reject_application(app_id,s.user_id,reason);
    char loc[64]; snprintf(loc,64,"/admin/elections/%d/applications",eid);
    redirect_with_flash(c,loc,"Application rejected.","info");
}

/* ================================================================
   ADMIN CANDIDATES
   ================================================================ */

void handle_admin_manage_candidates(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}

    db_update_election_statuses();
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT title,status FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char etitle[256]="",estatus[20]="";
    if(sqlite3_step(stmt)==SQLITE_ROW){
        const char *t=(const char*)sqlite3_column_text(stmt,0);
        const char *st=(const char*)sqlite3_column_text(stmt,1);
        if(t)strncpy(etitle,t,255);if(st)strncpy(estatus,st,19);
    }
    sqlite3_finalize(stmt);

    /* Candidates list */
    sqlite3_prepare_v2(db_get(),
        "SELECT candidate_id,name,cnic,description FROM candidates WHERE election_id=?;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    char cand_rows[16384]="";
    while(sqlite3_step(stmt)==SQLITE_ROW){
        int cid=sqlite3_column_int(stmt,0);
        const char *cn=(const char*)sqlite3_column_text(stmt,1);
        const char *cc=(const char*)sqlite3_column_text(stmt,2);
        const char *cd=(const char*)sqlite3_column_text(stmt,3);
        char cnic_fmt[20]=""; if(cc) format_cnic(cc,cnic_fmt);
        char del_btn[256]="";
        if(strcmp(estatus,"upcoming")==0)
            snprintf(del_btn,sizeof(del_btn),
                "<form method=\"POST\" action=\"/admin/candidates/%d/delete\" style=\"display:inline;\" "
                "onsubmit=\"return confirm('Remove candidate?');\"><button type=\"submit\" class=\"btn btn-sm btn-danger\">Remove</button></form>",cid);
        char row[1024];
        snprintf(row,sizeof(row),
            "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
            cn?cn:"",cnic_fmt,cd?cd:"",del_btn);
        strncat(cand_rows,row,sizeof(cand_rows)-strlen(cand_rows)-1);
    }
    sqlite3_finalize(stmt);

    /* Add candidate form (upcoming only â€” eligible voters not yet candidates) */
    char add_form[8192]="";
    if(strcmp(estatus,"upcoming")==0){
        sqlite3_prepare_v2(db_get(),
            "SELECT u.user_id, u.name, u.cnic FROM users u "
            "JOIN election_voters ev ON ev.user_id=u.user_id "
            "WHERE ev.election_id=? AND u.is_deleted=0 "
            "AND NOT EXISTS (SELECT 1 FROM candidates c WHERE c.user_id=u.user_id AND c.election_id=?) "
            "ORDER BY u.name;",
            -1,&stmt,NULL);
        sqlite3_bind_int(stmt,1,eid); sqlite3_bind_int(stmt,2,eid);
        char voter_opts[4096]="<option value=\"\">-- Select Voter --</option>";
        while(sqlite3_step(stmt)==SQLITE_ROW){
            int vid=sqlite3_column_int(stmt,0);
            const char *vn=(const char*)sqlite3_column_text(stmt,1);
            const char *vc=(const char*)sqlite3_column_text(stmt,2);
            char cnic_fmt[20]=""; if(vc) format_cnic(vc,cnic_fmt);
            char opt[256]; snprintf(opt,256,"<option value=\"%d\">%s (%s)</option>",vid,vn?vn:"",cnic_fmt);
            strncat(voter_opts,opt,sizeof(voter_opts)-strlen(voter_opts)-1);
        }
        sqlite3_finalize(stmt);
        snprintf(add_form,sizeof(add_form),
            "<div class=\"card-glass p-6\"><h3 class=\"h4 mb-4\">Add Candidate Directly</h3>"
            "<form method=\"POST\" action=\"/admin/elections/%d/candidates/add\">"
            "<div class=\"form-group\">"
            "<label class=\"form-label\">Select Eligible Voter</label>"
            "<select name=\"user_id\" class=\"form-control\">%s</select></div>"
            "<div class=\"form-group\">"
            "<label class=\"form-label\">Description</label>"
            "<textarea name=\"description\" class=\"form-control\" rows=\"3\"></textarea></div>"
            "<button type=\"submit\" class=\"btn btn-primary\">Add Candidate</button>"
            "</form></div>",eid,voter_opts);
    }

    char eid_s[16]; snprintf(eid_s,16,"%d",eid);
    char no_msg[64]=""; if(!cand_rows[0]) strcpy(no_msg,"<p class=\"text-gray\">No candidates yet.</p>");
    const char *k[]={"ELECTION_ID","ELECTION_TITLE","CANDIDATE_ROWS","NO_CAND_MSG","ADD_FORM","USER_NAME"};
    const char *v[]={eid_s,etitle,cand_rows,no_msg,add_form,s.user_name};
    send_html_page(c,"admin_manage_candidates.html",k,v,6);
}

void handle_admin_add_candidate(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char uid_s[16]="",desc[2048]="";
    form_get(hm,"user_id",uid_s,sizeof(uid_s));
    form_get(hm,"description",desc,sizeof(desc));
    int uid=atoi(uid_s);
    if(!uid){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
        redirect_with_flash(c,loc,"Must select a voter.","danger"); return;
    }
    char name[128]="",cnic[20]=""; int da,db2; char dc[32];
    db_get_user(uid,name,cnic,NULL,&da,&db2,dc);
    if(!db_is_eligible(eid,uid)){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
        redirect_with_flash(c,loc,"Selected user is not an eligible voter.","danger"); return;
    }
    if(db_user_is_candidate(uid,eid)){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
        redirect_with_flash(c,loc,"User is already a candidate.","warning"); return;
    }
    db_add_candidate(uid,cnic,name,desc,eid,"default-candidate.png",0);
    char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
    redirect_with_flash(c,loc,"Candidate added.","success");
}

void handle_admin_delete_candidate(struct mg_connection *c, struct mg_http_message *hm, int cid) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    /* Get election_id */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT election_id FROM candidates WHERE candidate_id=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,cid);
    int eid=0;
    if(sqlite3_step(stmt)==SQLITE_ROW) eid=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt);
    if(db_delete_candidate(cid)){
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
        redirect_with_flash(c,loc,"Candidate removed.","success");
    } else {
        char loc[64]; snprintf(loc,64,"/admin/elections/%d/candidates",eid);
        redirect_with_flash(c,loc,"Cannot remove candidate (election not upcoming).","warning");
    }
}

/* ================================================================
   ADMIN RESULTS
   ================================================================ */

void handle_admin_results(struct mg_connection *c, struct mg_http_message *hm, int eid) {
    /* Admin can see results anytime; reuse public results with admin session */
    /* Temporarily override status check by calling public handler */
    db_update_election_statuses();

    /* Force it through */
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    read_flash_cookies(hm);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),"SELECT title,description,status FROM elections WHERE election_id=? AND is_deleted=0;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);
    if(sqlite3_step(stmt)!=SQLITE_ROW){sqlite3_finalize(stmt);redirect_with_flash(c,"/admin/elections","Election not found.","danger");return;}
    char etitle[256]="",edesc[1024]="",estatus[20]="";
    const char *t=(const char*)sqlite3_column_text(stmt,0);
    const char *d=(const char*)sqlite3_column_text(stmt,1);
    const char *st=(const char*)sqlite3_column_text(stmt,2);
    if(t)strncpy(etitle,t,255);if(d)strncpy(edesc,d,1023);if(st)strncpy(estatus,st,19);
    sqlite3_finalize(stmt);

    int total_votes = db_get_total_votes_in_election(eid);
    const char *rsql =
        "SELECT c.candidate_id, c.name, c.description, COUNT(v.vote_id) as vc "
        "FROM candidates c LEFT JOIN votes v ON v.candidate_id=c.candidate_id "
        "WHERE c.election_id=? GROUP BY c.candidate_id ORDER BY vc DESC;";
    sqlite3_prepare_v2(db_get(),rsql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,eid);

    char winner_html[512]="", results_rows[16384]="";
    int idx=0;
    while(sqlite3_step(stmt)==SQLITE_ROW){
        const char *cn=(const char*)sqlite3_column_text(stmt,1);
        const char *cd=(const char*)sqlite3_column_text(stmt,2);
        int vc=sqlite3_column_int(stmt,3);
        char cname[128]="",cdesc[512]="";
        if(cn)strncpy(cname,cn,127);if(cd)strncpy(cdesc,cd,511);
        double pct=total_votes>0?(vc*100.0/total_votes):0.0;
        if(idx==0&&total_votes>0)
            snprintf(winner_html,sizeof(winner_html),
                "<div class=\"card mb-8\" style=\"background:var(--gradient-primary);color:white;padding:var(--space-10);text-align:center;\">"
                "<div style=\"font-size:var(--text-5xl);\">ðŸŽ‰</div>"
                "<h2 class=\"mb-4\" style=\"color:white;\">Winner: %s</h2>"
                "<p style=\"font-size:var(--text-2xl);opacity:0.9;\">%d votes (%.1f%%)</p></div>",cname,vc,pct);
        char row[1024];
        snprintf(row,sizeof(row),
            "<div><div style=\"display:flex;justify-content:space-between;align-items:center;margin-bottom:var(--space-3);\">"
            "<div style=\"display:flex;align-items:center;gap:var(--space-4);\">"
            "<div style=\"width:50px;height:50px;background:var(--gradient-primary);border-radius:50%;display:flex;align-items:center;justify-content:center;color:white;font-weight:bold;\">#%d</div>"
            "<div><h3 class=\"mb-1\">%s</h3><p class=\"text-sm text-gray\">%s</p></div></div>"
            "<div class=\"text-right\"><p class=\"fw-bold\" style=\"font-size:var(--text-2xl);color:var(--primary-600);\">%d</p><p class=\"text-sm text-gray\">votes</p></div></div>"
            "<div class=\"progress\"><div class=\"progress-bar\" data-width=\"%.1f\" style=\"width:%.1f%%;\">%.1f%%</div></div></div>",
            idx+1,cname, cdesc[0]?cdesc:"No description",vc,pct,pct,pct);
        strncat(results_rows,row,sizeof(results_rows)-strlen(results_rows)-1);
        idx++;
    }
    sqlite3_finalize(stmt);

    char tv_str[16]; snprintf(tv_str,16,"%d",total_votes);
    char tc_str[16]; snprintf(tc_str,16,"%d",idx);
    const char *stat_col=strcmp(estatus,"active")==0?"success":(strcmp(estatus,"upcoming")==0?"warning":"secondary");
    char no_results[256]="";
    if(!idx) strncpy(no_results,"<div class=\"card text-center\" style=\"padding:var(--space-12);\"><h3>No Candidates</h3><p class=\"text-gray\">No candidates yet.</p></div>",sizeof(no_results));

    const char *k[]={"ELECTION_TITLE","ELECTION_DESC","ELECTION_STATUS","STATUS_COLOR",
                      "TOTAL_VOTES","TOTAL_CANDIDATES","WINNER_HTML","RESULTS_ROWS","NO_RESULTS","BACK_URL"};
    const char *v[]={etitle,edesc,estatus,stat_col,tv_str,tc_str,winner_html,results_rows,no_results,"/admin/elections"};
    send_html_page(c,"results.html",k,v,10);
}

/* ================================================================
   ADMIN CHANGE PASSWORD
   ================================================================ */

void handle_admin_change_password_get(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    const char *k[]={"USER_NAME"}; const char *v[]={s.user_name};
    send_html_page(c,"admin_change_password.html",k,v,1);
}

void handle_admin_change_password_post(struct mg_connection *c, struct mg_http_message *hm) {
    Session s = get_session_from_request(hm);
    if(!s.valid||!s.is_admin){redirect_with_flash(c,"/login","Access denied.","danger");return;}
    char cur[128]="",newp[128]="",conf[128]="";
    form_get(hm,"current_password",cur,sizeof(cur));
    form_get(hm,"new_password",newp,sizeof(newp));
    form_get(hm,"confirm_password",conf,sizeof(conf));

    /* Verify current */
    char cur_hash[65]; sha256_string(cur,cur_hash);
    char cnic[20]="",email[120]="";
    int is_adm=0,is_del=0; char created[32]=""; char name[128]="";
    db_get_user(s.user_id,name,cnic,email,&is_adm,&is_del,created);
    int uid2,adm2,del2;
    int ok=db_verify_user_by_email(email,cur_hash,&uid2,&adm2,&del2);
    if(!ok){redirect_with_flash(c,"/admin/change-password","Current password is incorrect.","danger");return;}
    if(strlen(newp)<8){redirect_with_flash(c,"/admin/change-password","New password must be at least 8 characters.","danger");return;}
    if(strcmp(newp,conf)!=0){redirect_with_flash(c,"/admin/change-password","New passwords do not match.","danger");return;}
    if(strcmp(cur,newp)==0){redirect_with_flash(c,"/admin/change-password","New password must differ from current.","danger");return;}

    char new_hash[65]; sha256_string(newp,new_hash);
    db_update_password(s.user_id, new_hash);
    session_destroy(db_get(), s.session_id);
    char cpw_hdr[512];
    snprintf(cpw_hdr, sizeof(cpw_hdr),
        "Set-Cookie: %s=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n"
        "Set-Cookie: flash_msg=Password%%20changed%%20successfully%%21%%20Please%%20log%%20in%%20again.; Path=/\r\n"
        "Set-Cookie: flash_type=success; Path=/\r\n"
        "Location: /login\r\n", SESSION_COOKIE_NAME);
    mg_http_reply(c, 302, cpw_hdr, "");
}

/* ================================================================
   FORGOT PASSWORD
   ================================================================ */

/* Send email notification via PowerShell (uses smtp_config.txt) */
static void send_email_notification(const char *to_email, const char *voter_name,
                                     const char *new_password) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "powershell -NonInteractive -WindowStyle Hidden -Command \""
        "try {"
        "$cfg = Get-Content 'smtp_config.txt' -ErrorAction Stop | ConvertFrom-StringData;"
        "$pass = ConvertTo-SecureString $cfg.smtp_pass -AsPlainText -Force;"
        "$cred = New-Object System.Management.Automation.PSCredential($cfg.smtp_user, $pass);"
        "Send-MailMessage -SmtpServer $cfg.smtp_server -Port $cfg.smtp_port -UseSsl "
        "-Credential $cred -From $cfg.smtp_from "
        "-To '%s' "
        "-Subject 'Password Reset - Online Voting System' "
        "-Body 'Dear %s,\\n\\nYour password has been reset by the administrator.\\n\\n"
        "New Password: %s\\n\\nPlease login and change your password immediately.\\n\\n"
        "Login at http://127.0.0.1:5000\\n\\nVoting System Admin';"
        "} catch { Write-Host $_.Exception.Message }\"",
        to_email, voter_name, new_password);
    system(cmd);
}

void handle_forgot_password_get(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    /* If already logged in, redirect away */
    Session s = get_session_from_request(hm);
    if (s.valid) {
        send_redirect(c, s.is_admin ? "/admin/dashboard" : "/dashboard");
        return;
    }
    send_html_page(c, "forgot_password.html", NULL, NULL, 0);
}

void handle_forgot_password_post(struct mg_connection *c, struct mg_http_message *hm) {
    char cnic[32]="", email[128]="";
    form_get(hm, "cnic",  cnic,  sizeof(cnic));
    form_get(hm, "email", email, sizeof(email));
    clean_cnic(cnic);

    if (strlen(cnic) != 13 || strlen(email) < 5 || !strchr(email,'@')) {
        redirect_with_flash(c, "/forgot-password",
            "Please enter your 13-digit CNIC and registered email.", "danger");
        return;
    }

    /* Look up user by CNIC */
    const char *sql =
        "SELECT user_id, name, email FROM users "
        "WHERE cnic=? AND is_admin=0 AND is_deleted=0;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, cnic, -1, SQLITE_TRANSIENT);
    int uid = 0; char dbname[128]="", dbemail[128]="";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        uid = sqlite3_column_int(stmt, 0);
        const char *n = (const char*)sqlite3_column_text(stmt, 1);
        const char *e = (const char*)sqlite3_column_text(stmt, 2);
        if (n) strncpy(dbname,  n, 127);
        if (e) strncpy(dbemail, e, 127);
    }
    sqlite3_finalize(stmt);

    if (!uid) {
        redirect_with_flash(c, "/forgot-password",
            "No account found with that CNIC.", "danger");
        return;
    }

    /* Verify email matches */
    if (strcasecmp(email, dbemail) != 0) {
        redirect_with_flash(c, "/forgot-password",
            "Email does not match our records for this CNIC.", "danger");
        return;
    }

    /* Create reset request */
    db_create_password_reset_request(uid, dbemail);
    redirect_with_flash(c, "/login",
        "Your password reset request has been forwarded to the admin. "
        "You will be updated with your new password through your registered email shortly.",
        "info");
}

/* ================================================================
   ADMIN - PASSWORD RESET REQUESTS
   ================================================================ */

typedef struct { char buf[32768]; } ResetBuf;
static void reset_row_cb(int req_id, int uid, const char *name,
                          const char *email, const char *cnic,
                          const char *requested_at, const char *status,
                          void *ud) {
    ResetBuf *rb = (ResetBuf*)ud; (void)uid;
    const char *badge = strcmp(status,"pending")==0 ? "warning" : "success";
    char cnic_fmt[20]=""; format_cnic(cnic, cnic_fmt);
    char row[1024];
    if (strcmp(status,"pending")==0) {
        snprintf(row, sizeof(row),
            "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td><span class=\"badge badge-%s\">%s</span></td>"
            "<td>"
            "<form method=\"POST\" action=\"/admin/password-resets/%d/resolve\" "
            "style=\"display:inline;\">"
            "<input name=\"new_password\" class=\"form-control\" style=\"width:140px;display:inline;\" "
            "placeholder=\"New password\" required minlength=\"6\"> "
            "<button class=\"btn btn-sm btn-primary\">Reset &amp; Email</button>"
            "</form>"
            "</td></tr>",
            name, cnic_fmt, email, requested_at, badge, status, req_id);
    } else {
        snprintf(row, sizeof(row),
            "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td><span class=\"badge badge-%s\">%s</span></td>"
            "<td><span class=\"text-gray\">Resolved</span></td></tr>",
            name, cnic_fmt, email, requested_at, badge, status);
    }
    strncat(rb->buf, row, sizeof(rb->buf)-strlen(rb->buf)-1);
}

void handle_admin_password_resets(struct mg_connection *c, struct mg_http_message *hm) {
    read_flash_cookies(hm);
    Session s = get_session_from_request(hm);
    if (!s.valid || !s.is_admin) {
        redirect_with_flash(c, "/login", "Access denied.", "danger"); return;
    }
    ResetBuf rb; memset(&rb, 0, sizeof(rb));
    db_foreach_reset_request(reset_row_cb, &rb);

    const char *no_msg = rb.buf[0] ? "" :
        "<div class=\"alert alert-info\">No password reset requests yet.</div>";
    const char *table_start = rb.buf[0] ?
        "<div class=\"table-responsive\"><table class=\"table\">"
        "<thead><tr><th>Voter</th><th>CNIC</th><th>Email</th>"
        "<th>Requested</th><th>Status</th><th>Action</th></tr></thead>"
        "<tbody>" : "";
    const char *table_end = rb.buf[0] ? "</tbody></table></div>" : "";

    char full[40000];
    snprintf(full, sizeof(full), "%s%s%s%s", no_msg, table_start, rb.buf, table_end);

    const char *keys[] = {"RESET_TABLE","USER_NAME"};
    const char *vals[] = {full, s.user_name};
    send_html_page(c, "admin_password_resets.html", keys, vals, 2);
}

void handle_admin_resolve_reset(struct mg_connection *c, struct mg_http_message *hm, int req_id) {
    Session s = get_session_from_request(hm);
    if (!s.valid || !s.is_admin) {
        redirect_with_flash(c, "/login", "Access denied.", "danger"); return;
    }
    char new_password[128]="";
    form_get(hm, "new_password", new_password, sizeof(new_password));
    if (strlen(new_password) < 6) {
        redirect_with_flash(c, "/admin/password-resets",
            "Password must be at least 6 characters.", "danger"); return;
    }

    /* Get voter email before resolving */
    const char *sel =
        "SELECT r.email, u.name FROM password_reset_requests r "
        "JOIN users u ON u.user_id=r.user_id WHERE r.req_id=?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(), sel, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, req_id);
    char voter_email[128]="", voter_name[128]="";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *e = (const char*)sqlite3_column_text(stmt, 0);
        const char *n = (const char*)sqlite3_column_text(stmt, 1);
        if (e) strncpy(voter_email, e, 127);
        if (n) strncpy(voter_name,  n, 127);
    }
    sqlite3_finalize(stmt);

    char hashed[65]; sha256_string(new_password, hashed);
    if (db_resolve_password_reset(req_id, hashed)) {
        export_all_data_xls();
        /* Send email notification */
        if (voter_email[0]) {
            send_email_notification(voter_email, voter_name, new_password);
        }
        redirect_with_flash(c, "/admin/password-resets",
            "Password reset successfully. Email notification sent.", "success");
    } else {
        redirect_with_flash(c, "/admin/password-resets",
            "Could not resolve request. It may already be resolved.", "danger");
    }
}



/* ================================================================
   ROUTER AND MAIN
   ================================================================ */


/* Helper macros for routing using mg_str fields */
#define URI_EQ(hm, path)     (strncmp((hm)->uri.buf, (path), (hm)->uri.len) == 0 && \
                               strlen(path) == (size_t)(hm)->uri.len)
#define URI_PREFIX(hm, pre)  (strncmp((hm)->uri.buf, (pre), strlen(pre)) == 0)
#define METHOD_IS(hm, m)     (strncmp((hm)->method.buf, (m), (hm)->method.len) == 0 && \
                               strlen(m) == (size_t)(hm)->method.len)
#define IS_GET(hm)           METHOD_IS(hm, "GET")
#define IS_POST(hm)          METHOD_IS(hm, "POST")

/* Extract integer segment from URI after a given prefix */
static int uri_int(struct mg_http_message *hm, const char *prefix) {
    int id = 0;
    const char *p = hm->uri.buf + strlen(prefix);
    sscanf(p, "%d", &id);
    return id;
}

/* Check if URI matches a prefix and optional suffix (e.g. prefix="/admin/voters/", suffix="/edit") */
static int uri_match_seg(const char *uri, size_t ulen, const char *prefix, const char *suffix) {
    size_t plen = strlen(prefix);
    size_t slen = suffix ? strlen(suffix) : 0;
    if (ulen < plen + 1) return 0;
    if (strncmp(uri, prefix, plen) != 0) return 0;
    if (slen == 0) return 1;  /* no suffix required */
    /* find suffix from end */
    if (ulen < plen + slen) return 0;
    return strncmp(uri + ulen - slen, suffix, slen) == 0;
}

static void route(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;

    const char *uri = hm->uri.buf;
    size_t ulen = hm->uri.len;

    /* Serve static files (style.css, app.js) from root */
    if ((ulen == 10 && strncmp(uri, "/style.css", 10) == 0) ||
        (ulen == 7  && strncmp(uri, "/app.js", 7) == 0) ||
        strncmp(uri, "/images/", 8) == 0 ||
        (ulen == 12 && strncmp(uri, "/favicon.ico", 12) == 0)) {
        struct mg_http_serve_opts opts = {0};
        opts.root_dir = ".";
        mg_http_serve_dir(c, hm, &opts);
        return;
    }

    /* ---- AUTH ---- */
    if (URI_EQ(hm, "/login") && IS_GET(hm)) {
        handle_login_get(c, hm);
    } else if (URI_EQ(hm, "/login") && IS_POST(hm)) {
        handle_login_post(c, hm);
    } else if (URI_EQ(hm, "/logout")) {
        handle_logout(c, hm);
    } else if (URI_EQ(hm, "/forgot-password") && IS_GET(hm)) {
        handle_forgot_password_get(c, hm);
    } else if (URI_EQ(hm, "/forgot-password") && IS_POST(hm)) {
        handle_forgot_password_post(c, hm);

    /* ---- VOTER DASHBOARD ---- */
    } else if (URI_EQ(hm, "/dashboard")) {
        handle_voter_dashboard(c, hm);
    } else if (URI_EQ(hm, "/vote") && IS_POST(hm)) {
        handle_cast_vote(c, hm);

    /* ---- RESULTS ---- */
    } else if (strncmp(uri, "/results/", 9) == 0) {
        int eid = uri_int(hm, "/results/");
        handle_public_results(c, hm, eid);

    /* ---- ELECTION DETAILS & APPLY ---- */
    } else if (uri_match_seg(uri, ulen, "/election/", "/apply") && IS_GET(hm)) {
        int eid = uri_int(hm, "/election/");
        handle_apply_get(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/election/", "/apply") && IS_POST(hm)) {
        int eid = uri_int(hm, "/election/");
        handle_apply_post(c, hm, eid);
    } else if (strncmp(uri, "/election/", 10) == 0) {
        int eid = uri_int(hm, "/election/");
        handle_election_details(c, hm, eid);
    } else if (strncmp(uri, "/applications/", 14) == 0) {
        int aid = uri_int(hm, "/applications/");
        handle_view_application(c, hm, aid);

    /* ---- ADMIN DASHBOARD ---- */
    } else if (URI_EQ(hm, "/admin/dashboard")) {
        handle_admin_dashboard(c, hm);
    } else if (URI_EQ(hm, "/admin/change-password") && IS_GET(hm)) {
        handle_admin_change_password_get(c, hm);
    } else if (URI_EQ(hm, "/admin/change-password") && IS_POST(hm)) {
        handle_admin_change_password_post(c, hm);
    } else if (URI_EQ(hm, "/admin/password-resets")) {
        handle_admin_password_resets(c, hm);
    } else if (uri_match_seg(uri, ulen, "/admin/password-resets/", "/resolve") && IS_POST(hm)) {
        int rid = uri_int(hm, "/admin/password-resets/");
        handle_admin_resolve_reset(c, hm, rid);

    /* ---- ADMIN VOTERS ---- */
    } else if (URI_EQ(hm, "/admin/voters/add") && IS_GET(hm)) {
        handle_admin_add_voter_get(c, hm);
    } else if (URI_EQ(hm, "/admin/voters/add") && IS_POST(hm)) {
        handle_admin_add_voter_post(c, hm);
    } else if (uri_match_seg(uri, ulen, "/admin/voters/", "/edit") && IS_GET(hm)) {
        int uid = uri_int(hm, "/admin/voters/");
        handle_admin_edit_voter_get(c, hm, uid);
    } else if (uri_match_seg(uri, ulen, "/admin/voters/", "/edit") && IS_POST(hm)) {
        int uid = uri_int(hm, "/admin/voters/");
        handle_admin_edit_voter_post(c, hm, uid);
    } else if (uri_match_seg(uri, ulen, "/admin/voters/", "/ban") && IS_POST(hm)) {
        int uid = uri_int(hm, "/admin/voters/");
        handle_admin_ban_voter(c, hm, uid);
    } else if (uri_match_seg(uri, ulen, "/admin/voters/", "/unban") && IS_POST(hm)) {
        int uid = uri_int(hm, "/admin/voters/");
        handle_admin_unban_voter(c, hm, uid);
    } else if (uri_match_seg(uri, ulen, "/admin/voters/", "/delete") && IS_POST(hm)) {
        int uid = uri_int(hm, "/admin/voters/");
        handle_admin_delete_voter_permanent(c, hm, uid);
    } else if (URI_EQ(hm, "/admin/voters")) {
        handle_admin_voters(c, hm);

    /* ---- ADMIN ELECTIONS ---- */
    } else if (URI_EQ(hm, "/admin/elections/create") && IS_GET(hm)) {
        handle_admin_create_election_get(c, hm);
    } else if (URI_EQ(hm, "/admin/elections/create") && IS_POST(hm)) {
        handle_admin_create_election_post(c, hm);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/candidates/add") && IS_POST(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_add_candidate(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/candidates")) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_manage_candidates(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/applications")) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_election_applications(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/voters") && IS_GET(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_election_voters_get(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/voters") && IS_POST(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_election_voters_post(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/delete") && IS_POST(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_delete_election(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/edit") && IS_GET(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_edit_election_get(c, hm, eid);
    } else if (uri_match_seg(uri, ulen, "/admin/elections/", "/edit") && IS_POST(hm)) {
        int eid = uri_int(hm, "/admin/elections/");
        handle_admin_edit_election_post(c, hm, eid);
    } else if (strncmp(uri, "/admin/results/", 15) == 0) {
        int eid = uri_int(hm, "/admin/results/");
        handle_admin_results(c, hm, eid);
    } else if (URI_EQ(hm, "/admin/elections")) {
        handle_admin_elections(c, hm);

    /* ---- ADMIN APPLICATIONS ---- */
    } else if (uri_match_seg(uri, ulen, "/admin/applications/", "/approve") && IS_POST(hm)) {
        int aid = uri_int(hm, "/admin/applications/");
        handle_admin_approve_application(c, hm, aid);
    } else if (uri_match_seg(uri, ulen, "/admin/applications/", "/reject") && IS_POST(hm)) {
        int aid = uri_int(hm, "/admin/applications/");
        handle_admin_reject_application(c, hm, aid);
    } else if (URI_EQ(hm, "/admin/applications")) {
        handle_admin_applications(c, hm);

    /* ---- ADMIN CANDIDATES ---- */
    } else if (uri_match_seg(uri, ulen, "/admin/candidates/", "/delete") && IS_POST(hm)) {
        int cid = uri_int(hm, "/admin/candidates/");
        handle_admin_delete_candidate(c, hm, cid);

    /* ---- ROOT redirect ---- */
    } else if (ulen == 1 && uri[0] == '/') {
        Session s = get_session_from_request(hm);
        if (s.valid) {
            const char *dest = s.is_admin ? "/admin/dashboard" : "/dashboard";
            mg_http_reply(c, 302, "Location: %s\r\n", "", dest);
        } else {
            mg_http_reply(c, 302, "Location: /login\r\n", "");
        }
    } else {
        mg_http_reply(c, 404, "Content-Type: text/html\r\n",
            "<h1 style='font-family:sans-serif;text-align:center;margin-top:4rem'>404 - Page Not Found</h1>"
            "<p style='text-align:center'><a href='/'>Go Home</a></p>");
    }
}

static void handle_cli(int argc, char *argv[]) {
    if (argc < 3) return;
    if (strcmp(argv[2], "update-admin") == 0 && argc >= 6) {
        const char *cnic  = argv[3];
        const char *email = argv[4];
        const char *pass  = argv[5];
        char hash[65]; sha256_string(pass, hash);
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db_get(),
            "SELECT user_id FROM users WHERE is_admin=1;", -1, &stmt, NULL);
        int exists = (sqlite3_step(stmt) == SQLITE_ROW);
        int uid = exists ? sqlite3_column_int(stmt, 0) : 0;
        sqlite3_finalize(stmt);
        if (exists) {
            db_update_password(uid, hash);
            sqlite3_stmt *s2;
            sqlite3_prepare_v2(db_get(),
                "UPDATE users SET email=?, cnic=? WHERE user_id=?;", -1, &s2, NULL);
            sqlite3_bind_text(s2, 1, email, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(s2, 2, cnic,  -1, SQLITE_TRANSIENT);
            sqlite3_bind_int( s2, 3, uid);
            sqlite3_step(s2); sqlite3_finalize(s2);
        } else {
            db_create_user(cnic, "Administrator", email, hash, 1);
        }
        printf("Admin updated successfully.\n");
    } else if (strcmp(argv[2], "reset-db") == 0) {
        db_reset_all();
        printf("Database reset.\n");
    } else if (strcmp(argv[2], "clear-voters") == 0) {
        db_clear_voters();
        printf("Voters cleared.\n");
    } else if (strcmp(argv[2], "clear-elections") == 0) {
        db_clear_elections();
        printf("Elections cleared.\n");
    }
}

/* ================================================================
   EXCEL AUTO-EXPORT (data/ folder)
   Generates a single master styled HTML table with .xls extension
   that opens perfectly in Excel.
   ================================================================ */

static const char *XLS_HEADER =
    "<html xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
    "xmlns:x=\"urn:schemas-microsoft-com:office:excel\">"
    "<head><meta charset=\"UTF-8\">"
    "<style>"
    "body { font-family: Calibri, Arial, sans-serif; }"
    "table { border-collapse: collapse; width: 100%; margin-bottom: 30px; }"
    "th { background-color: #1F4E79; color: white; font-weight: bold; "
    "padding: 10px 12px; border: 1px solid #BDD7EE; text-align: left; }"
    "td { padding: 8px 12px; border: 1px solid #BDD7EE; }"
    "tr:nth-child(even) { background-color: #D6E4F0; }"
    "tr:nth-child(odd) { background-color: #FFFFFF; }"
    "tr:hover { background-color: #BDD7EE; }"
    "h1 { color: #1F4E79; font-size: 22px; margin-top: 30px; margin-bottom: 5px; border-bottom: 2px solid #1F4E79; }"
    "h2 { color: #2E75B6; font-size: 13px; font-weight: normal; margin-bottom: 15px; }"
    "</style></head><body>";

static const char *XLS_FOOTER = "</body></html>";

static void ensure_data_dir(void) {
    system("mkdir data >nul 2>&1");
}

void export_all_data_xls(void) {
    ensure_data_dir();
    FILE *f = fopen("data/System_Data.xls", "w");
    if (!f) return;
    fprintf(f, "%s\n", XLS_HEADER);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);
    fprintf(f, "<h2 style=\"font-size:16px; color:red;\">Report auto-generated at: %s</h2>\n", ts);

    sqlite3_stmt *stmt;

    /* ---- 1. ELECTIONS ---- */
    fprintf(f, "<h1>1. Elections Overview</h1>\n");
    fprintf(f, "<table><thead><tr>"
        "<th>ID</th><th>Title</th><th>Description</th><th>Status</th>"
        "<th>Start Date</th><th>End Date</th><th>Eligible Voters</th><th>Total Votes</th>"
        "</tr></thead><tbody>\n");
    sqlite3_prepare_v2(db_get(),
        "SELECT e.election_id, e.title, e.description, e.status, "
        "e.start_date, e.end_date, "
        "(SELECT COUNT(*) FROM election_voters WHERE election_id=e.election_id), "
        "(SELECT COUNT(*) FROM votes WHERE election_id=e.election_id) "
        "FROM elections e WHERE e.is_deleted=0 ORDER BY e.created_at DESC;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int eid = sqlite3_column_int(stmt, 0);
        const char *t1  = (const char*)sqlite3_column_text(stmt, 1);
        const char *d  = (const char*)sqlite3_column_text(stmt, 2);
        const char *st = (const char*)sqlite3_column_text(stmt, 3);
        const char *sd = (const char*)sqlite3_column_text(stmt, 4);
        const char *ed = (const char*)sqlite3_column_text(stmt, 5);
        int ev = sqlite3_column_int(stmt, 6);
        int tv = sqlite3_column_int(stmt, 7);
        const char *sc = (st && strcmp(st,"active")==0) ? "green" :
                         (st && strcmp(st,"upcoming")==0) ? "#E67E22" : "gray";
        fprintf(f, "<tr><td>%d</td><td>%s</td><td>%s</td>"
            "<td style=\"color:%s;font-weight:bold;\">%s</td>"
            "<td>%s</td><td>%s</td><td>%d</td><td>%d</td></tr>\n",
            eid, t1?t1:"", d?d:"", sc, st?st:"", sd?sd:"", ed?ed:"", ev, tv);
    }
    sqlite3_finalize(stmt);
    fprintf(f, "</tbody></table>\n");

    /* ---- 2. VOTERS ---- */
    fprintf(f, "<h1>2. Registered Voters</h1>\n");
    fprintf(f, "<table><thead><tr>"
        "<th>ID</th><th>Name</th><th>CNIC</th><th>Email</th>"
        "<th>Status</th><th>Registered At</th>"
        "</tr></thead><tbody>\n");
    sqlite3_prepare_v2(db_get(),
        "SELECT user_id,name,cnic,email,is_deleted,created_at "
        "FROM users WHERE is_admin=0 ORDER BY created_at DESC;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int uid = sqlite3_column_int(stmt, 0);
        const char *nm = (const char*)sqlite3_column_text(stmt, 1);
        const char *cn = (const char*)sqlite3_column_text(stmt, 2);
        const char *em = (const char*)sqlite3_column_text(stmt, 3);
        int banned = sqlite3_column_int(stmt, 4);
        const char *dt = (const char*)sqlite3_column_text(stmt, 5);
        char cnic_fmt[20] = "";
        if (cn) format_cnic(cn, cnic_fmt);
        fprintf(f, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td style=\"color:%s;font-weight:bold;\">%s</td><td>%s</td></tr>\n",
            uid, nm?nm:"", cnic_fmt, em?em:"",
            banned?"red":"green", banned?"BANNED":"Active", dt?dt:"");
    }
    sqlite3_finalize(stmt);
    fprintf(f, "</tbody></table>\n");

    /* ---- 3. CANDIDATES ---- */
    fprintf(f, "<h1>3. Approved Candidates</h1>\n");
    fprintf(f, "<table><thead><tr>"
        "<th>ID</th><th>Election</th><th>Name</th><th>CNIC</th>"
        "<th>Description</th><th>Votes Received</th>"
        "</tr></thead><tbody>\n");
    sqlite3_prepare_v2(db_get(),
        "SELECT c.candidate_id, e.title, c.name, c.cnic, c.description, "
        "(SELECT COUNT(*) FROM votes v WHERE v.candidate_id=c.candidate_id) "
        "FROM candidates c "
        "JOIN elections e ON e.election_id=c.election_id "
        "ORDER BY e.title, c.name;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int cid = sqlite3_column_int(stmt, 0);
        const char *et = (const char*)sqlite3_column_text(stmt, 1);
        const char *cn = (const char*)sqlite3_column_text(stmt, 2);
        const char *cc = (const char*)sqlite3_column_text(stmt, 3);
        const char *cd = (const char*)sqlite3_column_text(stmt, 4);
        int vc = sqlite3_column_int(stmt, 5);
        char cnic_fmt[20] = "";
        if (cc) format_cnic(cc, cnic_fmt);
        fprintf(f, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td>%s</td><td><b>%d</b></td></tr>\n",
            cid, et?et:"", cn?cn:"", cnic_fmt, cd?cd:"", vc);
    }
    sqlite3_finalize(stmt);
    fprintf(f, "</tbody></table>\n");

    /* ---- 4. APPLICATIONS ---- */
    fprintf(f, "<h1>4. Candidate Applications</h1>\n");
    fprintf(f, "<table><thead><tr>"
        "<th>ID</th><th>Election</th><th>Applicant</th><th>CNIC</th>"
        "<th>Status</th><th>Applied At</th>"
        "</tr></thead><tbody>\n");
    sqlite3_prepare_v2(db_get(),
        "SELECT a.application_id, e.title, u.name, u.cnic, "
        "a.status, a.applied_at "
        "FROM candidate_applications a "
        "JOIN users u ON u.user_id=a.user_id "
        "JOIN elections e ON e.election_id=a.election_id "
        "ORDER BY a.applied_at DESC;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int aid = sqlite3_column_int(stmt, 0);
        const char *et = (const char*)sqlite3_column_text(stmt, 1);
        const char *an = (const char*)sqlite3_column_text(stmt, 2);
        const char *ac = (const char*)sqlite3_column_text(stmt, 3);
        const char *as2 = (const char*)sqlite3_column_text(stmt, 4);
        const char *aat = (const char*)sqlite3_column_text(stmt, 5);
        char cnic_fmt[20] = "";
        if (ac) format_cnic(ac, cnic_fmt);
        const char *sc = (as2 && strcmp(as2,"approved")==0) ? "green" :
                         (as2 && strcmp(as2,"pending")==0) ? "#E67E22" : "red";
        fprintf(f, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td>"
            "<td style=\"color:%s;font-weight:bold;\">%s</td>"
            "<td>%s</td></tr>\n",
            aid, et?et:"", an?an:"", cnic_fmt, sc, as2?as2:"", aat?aat:"");
    }
    sqlite3_finalize(stmt);
    fprintf(f, "</tbody></table>\n");

    /* ---- 5. VOTING RECORDS ---- */
    fprintf(f, "<h1>5. Voting Record Log</h1>\n");
    fprintf(f, "<table><thead><tr>"
        "<th>Vote ID</th><th>Election</th><th>Voter Name</th>"
        "<th>Candidate Voted For</th><th>Voted At</th>"
        "</tr></thead><tbody>\n");
    sqlite3_prepare_v2(db_get(),
        "SELECT v.vote_id, e.title, u.name, c.name, v.voted_at "
        "FROM votes v "
        "JOIN elections e ON e.election_id=v.election_id "
        "JOIN users u ON u.user_id=v.user_id "
        "JOIN candidates c ON c.candidate_id=v.candidate_id "
        "ORDER BY v.voted_at DESC;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int vid = sqlite3_column_int(stmt, 0);
        const char *et = (const char*)sqlite3_column_text(stmt, 1);
        const char *vn = (const char*)sqlite3_column_text(stmt, 2);
        const char *cn = (const char*)sqlite3_column_text(stmt, 3);
        const char *va = (const char*)sqlite3_column_text(stmt, 4);
        fprintf(f, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
            vid, et?et:"", vn?vn:"", cn?cn:"", va?va:"");
    }
    sqlite3_finalize(stmt);
    fprintf(f, "</tbody></table>\n");

    fprintf(f, "%s", XLS_FOOTER);
    fclose(f);
}
int main(int argc, char *argv[]) {
    /* Ensure instance folder exists */
    system("mkdir instance >nul 2>&1");

    if (!db_init("instance/voting_system.db")) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    /* Bootstrap admin if none exists */
    sqlite3_stmt *chk;
    sqlite3_prepare_v2(db_get(),
        "SELECT COUNT(*) FROM users WHERE is_admin=1;", -1, &chk, NULL);
    int admin_count = 0;
    if (sqlite3_step(chk) == SQLITE_ROW) admin_count = sqlite3_column_int(chk, 0);
    sqlite3_finalize(chk);
    if (admin_count == 0) {
        char hash[65]; sha256_string("Admin@123", hash);
        db_create_user("0000000000000", "Administrator", "admin@votingsystem.com", hash, 1);
        printf("[*] Admin account created: admin@votingsystem.com / Admin@123\n");
    }

    /* Initial data export */
    export_all_data_xls();
    export_all_data_xls();
    export_all_data_xls();

    /* Handle CLI commands AFTER DB is open */
    if (argc >= 3 && strcmp(argv[1], "--cmd") == 0) {
        handle_cli(argc, argv);
        return 0;
    }

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_log_set(0);  /* silence debug output â€” errors only */
    mg_http_listen(&mgr, "http://0.0.0.0:5000", route, NULL);
    printf("=================================================\n");
    printf(" Voting System C Backend running on port 5000\n");
    printf(" URL: http://127.0.0.1:5000\n");
    printf(" Admin: admin@votingsystem.com / Admin@123\n");
    printf("=================================================\n");
    printf(" Press Ctrl+C to stop\n\n");

    for (;;) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    return 0;
}

