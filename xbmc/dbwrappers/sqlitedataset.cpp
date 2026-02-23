/**********************************************************************
 *  Copyright (C) 2004, Leo Seib, Hannover
 *
 *  Project:SQLiteDataset C++ Dynamic Library
 *  Module: SQLiteDataset class realisation file
 *  Author: Leo Seib      E-Mail: leoseib@web.de
 *  Begin: 5/04/2002
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>

#include "sqlitedataset.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif

namespace {
#define X(VAL) std::make_pair(VAL, #VAL)
//!@todo Remove ifdefs when sqlite version requirement has been bumped to at least 3.26.0
const std::map<int, const char*> g_SqliteErrorStrings =
{
  X(SQLITE_OK),
  X(SQLITE_ERROR),
  X(SQLITE_INTERNAL),
  X(SQLITE_PERM),
  X(SQLITE_ABORT),
  X(SQLITE_BUSY),
  X(SQLITE_LOCKED),
  X(SQLITE_NOMEM),
  X(SQLITE_READONLY),
  X(SQLITE_INTERRUPT),
  X(SQLITE_IOERR),
  X(SQLITE_CORRUPT),
  X(SQLITE_NOTFOUND),
  X(SQLITE_FULL),
  X(SQLITE_CANTOPEN),
  X(SQLITE_PROTOCOL),
  X(SQLITE_EMPTY),
  X(SQLITE_SCHEMA),
  X(SQLITE_TOOBIG),
  X(SQLITE_CONSTRAINT),
  X(SQLITE_MISMATCH),
  X(SQLITE_MISUSE),
  X(SQLITE_NOLFS),
  X(SQLITE_AUTH),
  X(SQLITE_FORMAT),
  X(SQLITE_RANGE),
  X(SQLITE_NOTADB),
  X(SQLITE_NOTICE),
  X(SQLITE_WARNING),
  X(SQLITE_ROW),
  X(SQLITE_DONE),
#if defined(SQLITE_ERROR_MISSING_COLLSEQ)
  X(SQLITE_ERROR_MISSING_COLLSEQ),
#endif
#if defined(SQLITE_ERROR_RETRY)
  X(SQLITE_ERROR_RETRY),
#endif
#if defined(SQLITE_ERROR_SNAPSHOT)
  X(SQLITE_ERROR_SNAPSHOT),
#endif
  X(SQLITE_IOERR_READ),
  X(SQLITE_IOERR_SHORT_READ),
  X(SQLITE_IOERR_WRITE),
  X(SQLITE_IOERR_FSYNC),
  X(SQLITE_IOERR_DIR_FSYNC),
  X(SQLITE_IOERR_TRUNCATE),
  X(SQLITE_IOERR_FSTAT),
  X(SQLITE_IOERR_UNLOCK),
  X(SQLITE_IOERR_RDLOCK),
  X(SQLITE_IOERR_DELETE),
  X(SQLITE_IOERR_BLOCKED),
  X(SQLITE_IOERR_NOMEM),
  X(SQLITE_IOERR_ACCESS),
  X(SQLITE_IOERR_CHECKRESERVEDLOCK),
  X(SQLITE_IOERR_LOCK),
  X(SQLITE_IOERR_CLOSE),
  X(SQLITE_IOERR_DIR_CLOSE),
  X(SQLITE_IOERR_SHMOPEN),
  X(SQLITE_IOERR_SHMSIZE),
  X(SQLITE_IOERR_SHMLOCK),
  X(SQLITE_IOERR_SHMMAP),
  X(SQLITE_IOERR_SEEK),
  X(SQLITE_IOERR_DELETE_NOENT),
  X(SQLITE_IOERR_MMAP),
  X(SQLITE_IOERR_GETTEMPPATH),
  X(SQLITE_IOERR_CONVPATH),
#if defined(SQLITE_IOERR_VNODE)
  X(SQLITE_IOERR_VNODE),
#endif
#if defined(SQLITE_IOERR_AUTH)
  X(SQLITE_IOERR_AUTH),
#endif
#if defined(SQLITE_IOERR_BEGIN_ATOMIC)
  X(SQLITE_IOERR_BEGIN_ATOMIC),
#endif
#if defined(SQLITE_IOERR_COMMIT_ATOMIC)
  X(SQLITE_IOERR_COMMIT_ATOMIC),
#endif
#if defined(SQLITE_IOERR_ROLLBACK_ATOMIC)
  X(SQLITE_IOERR_ROLLBACK_ATOMIC),
#endif
  X(SQLITE_LOCKED_SHAREDCACHE),
#if defined(SQLITE_LOCKED_VTAB)
  X(SQLITE_LOCKED_VTAB),
#endif
  X(SQLITE_BUSY_RECOVERY),
  X(SQLITE_BUSY_SNAPSHOT),
  X(SQLITE_CANTOPEN_NOTEMPDIR),
  X(SQLITE_CANTOPEN_ISDIR),
  X(SQLITE_CANTOPEN_FULLPATH),
  X(SQLITE_CANTOPEN_CONVPATH),
#if defined(SQLITE_CANTOPEN_DIRTYWAL)
  X(SQLITE_CANTOPEN_DIRTYWAL),
#endif
  X(SQLITE_CORRUPT_VTAB),
#if defined(SQLITE_CORRUPT_SEQUENCE)
  X(SQLITE_CORRUPT_SEQUENCE),
#endif
  X(SQLITE_READONLY_RECOVERY),
  X(SQLITE_READONLY_CANTLOCK),
  X(SQLITE_READONLY_ROLLBACK),
  X(SQLITE_READONLY_DBMOVED),
#if defined(SQLITE_READONLY_CANTINIT)
  X(SQLITE_READONLY_CANTINIT),
#endif
#if defined(SQLITE_READONLY_DIRECTORY)
  X(SQLITE_READONLY_DIRECTORY),
#endif
  X(SQLITE_ABORT_ROLLBACK),
  X(SQLITE_CONSTRAINT_CHECK),
  X(SQLITE_CONSTRAINT_COMMITHOOK),
  X(SQLITE_CONSTRAINT_FOREIGNKEY),
  X(SQLITE_CONSTRAINT_FUNCTION),
  X(SQLITE_CONSTRAINT_NOTNULL),
  X(SQLITE_CONSTRAINT_PRIMARYKEY),
  X(SQLITE_CONSTRAINT_TRIGGER),
  X(SQLITE_CONSTRAINT_UNIQUE),
  X(SQLITE_CONSTRAINT_VTAB),
  X(SQLITE_CONSTRAINT_ROWID),
  X(SQLITE_NOTICE_RECOVER_WAL),
  X(SQLITE_NOTICE_RECOVER_ROLLBACK),
  X(SQLITE_WARNING_AUTOINDEX),
  X(SQLITE_AUTH_USER),
#if defined(SQLITE_OK_LOAD_PERMANENTLY)
  X(SQLITE_OK_LOAD_PERMANENTLY),
#endif
};
#undef X

#if defined(TARGET_SWITCH)
static int g_switchSqlitePathMode = 0; // 0=auto, 1=prefer sdmc:/, 2=prefer /...
static std::unordered_set<std::string> g_switchSqliteFallbackLogged;

static std::string NormalizeSwitchPath(std::string path)
{
  std::replace(path.begin(), path.end(), '\\', '/');
  return path;
}

static void EnsureParentDirsSwitch(const std::string& filePath)
{
  std::string path = NormalizeSwitchPath(filePath);
  size_t slash = path.find_last_of('/');
  if (slash == std::string::npos)
    return;

  std::string dir = path.substr(0, slash);
  if (dir.empty())
    return;

  size_t start = 0;
  std::string cur;
  if (StringUtils::StartsWith(dir, "sdmc:/"))
  {
    cur = "sdmc:/";
    start = 6;
  }
  else if (dir[0] == '/')
  {
    cur = "/";
    start = 1;
  }

  while (start < dir.size())
  {
    size_t next = dir.find('/', start);
    std::string part = (next == std::string::npos) ? dir.substr(start) : dir.substr(start, next - start);
    if (!part.empty())
    {
      if (!cur.empty() && cur.back() != '/')
        cur.push_back('/');
      cur += part;
      if (mkdir(cur.c_str(), 0777) != 0 && errno != EEXIST)
      {
        CLog::Log(LOGDEBUG, "SqliteDatabase: mkdir step failed for %s (errno=%d: %s)",
                  cur.c_str(), errno, strerror(errno));
      }
    }
    if (next == std::string::npos)
      break;
    start = next + 1;
  }
}

static bool ParentDirExistsSwitch(const std::string& filePath)
{
  std::string path = NormalizeSwitchPath(filePath);
  size_t slash = path.find_last_of('/');
  if (slash == std::string::npos)
    return false;
  std::string dir = path.substr(0, slash);
  if (dir.empty())
    return false;
  struct stat st;
  return stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
#endif
}

namespace dbiplus {
//************* Callback function ***************************

int callback(void* res_ptr,int ncol, char** result,char** cols)
{
  result_set* r = static_cast<result_set*>(res_ptr);

  if (!r->record_header.size())
  {
    r->record_header.reserve(ncol);
    for (int i=0; i < ncol; i++) {
      field_prop header;
      header.name = cols[i];
      r->record_header.push_back(header);
    }
  }

  if (result != NULL)
  {
    sql_record *rec = new sql_record;
    rec->resize(ncol);
    for (int i=0; i<ncol; i++)
    {
      field_value &v = rec->at(i);
      if (result[i] == NULL)
      {
        v.set_asString("");
        v.set_isNull();
      }
      else
      {
        v.set_asString(result[i]);
      }
    }
    r->records.push_back(rec);
  }
  return 0;
}

static int busy_callback(void*, int busyCount)
{
  Sleep(100);
  return 1;
}

//************* SqliteDatabase implementation ***************

SqliteDatabase::SqliteDatabase() {

  active = false;
  _in_transaction = false;    // for transaction

  error = "Unknown database error";//S_NO_CONNECTION;
  host = "localhost";
  port = "";
  db = "sqlite.db";
  login = "root";
  passwd = "";
}

SqliteDatabase::~SqliteDatabase() {
  disconnect();
}


Dataset* SqliteDatabase::CreateDataset() const {
  return new SqliteDataset(const_cast<SqliteDatabase*>(this));
}

void SqliteDatabase::setHostName(const char *newHost) {
  host = newHost;

  // hostname is the relative folder to the database, ensure it's slash terminated
  if (host[host.length()-1] != '/' && host[host.length()-1] != '\\')
    host += "/";

  // ensure the fully qualified path has slashes in the correct direction
  if ( (host[1] == ':') && isalpha(host[0]))
  {
    size_t pos = 0;
    while ( (pos = host.find("/", pos)) != std::string::npos )
      host.replace(pos++, 1, "\\");
  }
  else
  {
    size_t pos = 0;
    while ( (pos = host.find("\\", pos)) != std::string::npos )
      host.replace(pos++, 1, "/");
  }
}

void SqliteDatabase::setDatabase(const char *newDb) {
  db = newDb;

  // db is the filename for the database, ensure it's not slash prefixed
  if (newDb[0] == '/' || newDb[0] == '\\')
    db = db.substr(1);

  // ensure the ".db" extension is appended to the end
  if ( db.find(".db") != (db.length()-3) )
    db += ".db";
}

int SqliteDatabase::status(void) {
  if (active == false) return DB_CONNECTION_NONE;
  return DB_CONNECTION_OK;
}

int SqliteDatabase::setErr(int err_code, const char * qry) {
  std::stringstream ss;
  ss << "[" << db << "] ";
  auto errorIt = g_SqliteErrorStrings.find(err_code);
  if (errorIt != g_SqliteErrorStrings.end()) {
    ss << "SQLite error " << errorIt->second;
  } else {
    ss << "Undefined SQLite error " << err_code;
  }
  if (conn)
    ss << " (" << sqlite3_errmsg(conn) << ")";
  ss << "\nQuery: " << qry;
  error = ss.str();
  return err_code;
}

const char *SqliteDatabase::getErrorMsg() {
   return error.c_str();
}

int SqliteDatabase::connect(bool create) {
  if (host.empty() || db.empty())
    return DB_CONNECTION_NONE;

  //CLog::Log(LOGDEBUG, "Connecting to sqlite:%s:%s", host.c_str(), db.c_str());

  std::string db_fullpath = URIUtils::AddFileToFolder(host, db);

  try
  {
    disconnect();
    int flags = SQLITE_OPEN_READWRITE;
    if (create)
      flags |= SQLITE_OPEN_CREATE;
    int errorCode = SQLITE_OK;
#if defined(TARGET_SWITCH)
    std::vector<std::string> candidatePaths;
    const std::string canonical = NormalizeSwitchPath(db_fullpath);
    const std::string altPath =
      StringUtils::StartsWith(canonical, "sdmc:/") ? canonical.substr(5) :
      ((!canonical.empty() && canonical[0] == '/') ? ("sdmc:" + canonical) : std::string());
    std::string userdataAbsPath;
    std::string userdataRelPath;
    const size_t userdataPos = canonical.find("/userdata/");
    if (userdataPos != std::string::npos)
    {
      userdataAbsPath = canonical.substr(userdataPos);       // "/userdata/Database/..."
      userdataRelPath = canonical.substr(userdataPos + 1);   // "userdata/Database/..."
    }

    auto addUniquePath = [&candidatePaths](const std::string& p)
    {
      if (p.empty())
        return;
      if (std::find(candidatePaths.begin(), candidatePaths.end(), p) == candidatePaths.end())
        candidatePaths.push_back(p);
    };

    if (g_switchSqlitePathMode == 1)
    {
      addUniquePath(canonical);
      addUniquePath(altPath);
      addUniquePath(userdataAbsPath);
      addUniquePath(userdataRelPath);
    }
    else if (g_switchSqlitePathMode == 2)
    {
      addUniquePath(altPath);
      addUniquePath(canonical);
      addUniquePath(userdataAbsPath);
      addUniquePath(userdataRelPath);
    }
    else
    {
      // Eden currently behaves better with /switch/... sqlite paths.
      if (g_switchSqlitePathMode == 0 && StringUtils::StartsWith(canonical, "sdmc:/switch/"))
        g_switchSqlitePathMode = 2;

      if (!userdataAbsPath.empty())
      {
        addUniquePath(userdataAbsPath);
        addUniquePath(userdataRelPath);
      }

      if (!altPath.empty() && ParentDirExistsSwitch(altPath) && !ParentDirExistsSwitch(canonical))
      {
        addUniquePath(altPath);
        addUniquePath(canonical);
      }
      else
      {
        addUniquePath(canonical);
        addUniquePath(altPath);
      }
    }

    EnsureParentDirsSwitch(canonical);
    for (const auto& candidatePath : candidatePaths)
      EnsureParentDirsSwitch(candidatePath);

    for (const auto& candidatePath : candidatePaths)
    {
      const bool isLastCandidate = (&candidatePath == &candidatePaths.back());
      if (conn)
      {
        sqlite3_close(conn);
        conn = NULL;
      }

      // Use URI + nolock to bypass file-lock calls that are not supported by some Switch FS layers/emulators.
      std::string uriPath = "file:" + candidatePath + "?nolock=1";
      errorCode = sqlite3_open_v2(uriPath.c_str(), &conn, flags | SQLITE_OPEN_URI, NULL);
      if (errorCode != SQLITE_OK)
      {
        // Fallback to non-URI open if URI mode is unavailable.
        errorCode = sqlite3_open_v2(candidatePath.c_str(), &conn, flags, NULL);
      }

      if (errorCode == SQLITE_OK)
      {
        g_switchSqlitePathMode = StringUtils::StartsWith(candidatePath, "sdmc:/") ? 1 : 2;
        // Keep journal/temp activity in-memory on Switch to reduce FS lock pressure.
        char* pragmaErr = NULL;
        sqlite3_exec(conn, "PRAGMA journal_mode=MEMORY", NULL, NULL, &pragmaErr);
        if (pragmaErr)
        {
          CLog::Log(LOGWARNING, "SqliteDatabase: PRAGMA journal_mode failed for %s: %s",
                    candidatePath.c_str(), pragmaErr);
          sqlite3_free(pragmaErr);
          pragmaErr = NULL;
        }
        sqlite3_exec(conn, "PRAGMA temp_store=MEMORY", NULL, NULL, &pragmaErr);
        if (pragmaErr)
        {
          CLog::Log(LOGWARNING, "SqliteDatabase: PRAGMA temp_store failed for %s: %s",
                    candidatePath.c_str(), pragmaErr);
          sqlite3_free(pragmaErr);
          pragmaErr = NULL;
        }

        if (candidatePath != canonical)
        {
          const std::string key = canonical + "->" + candidatePath;
          if (g_switchSqliteFallbackLogged.insert(key).second)
            CLog::Log(LOGNOTICE, "SqliteDatabase: using path {} for {}", candidatePath, canonical);
          else
            CLog::Log(LOGDEBUG, "SqliteDatabase: using path {} for {}", candidatePath, canonical);
        }
        break;
      }

      int extCode = conn ? sqlite3_extended_errcode(conn) : errorCode;
      const char* errMsg = conn ? sqlite3_errmsg(conn) : sqlite3_errstr(errorCode);
      CLog::Log(isLastCandidate ? LOGERROR : LOGDEBUG,
                "SqliteDatabase: open failed for %s (rc=%d ext=%d msg=%s flags=0x%x create=%d)",
                candidatePath.c_str(), errorCode, extCode, errMsg ? errMsg : "unknown", flags, create ? 1 : 0);
    }
#else
    errorCode = sqlite3_open_v2(db_fullpath.c_str(), &conn, flags, NULL);
#endif
#if defined(TARGET_SWITCH)
    if (create && errorCode != SQLITE_OK)
    {
      CLog::Log(LOGERROR, "SqliteDatabase: create failed (rc=%d), falling back to in-memory database for %s", errorCode, db_fullpath.c_str());
      if (conn)
      {
        sqlite3_close(conn);
        conn = NULL;
      }
      errorCode = sqlite3_open(":memory:", &conn);
      if (errorCode == SQLITE_OK)
      {
        CLog::Log(LOGERROR, "SqliteDatabase: in-memory fallback active for %s", db_fullpath.c_str());
      }
      else
      {
        int memExt = conn ? sqlite3_extended_errcode(conn) : errorCode;
        const char* memMsg = conn ? sqlite3_errmsg(conn) : sqlite3_errstr(errorCode);
        CLog::Log(LOGERROR, "SqliteDatabase: in-memory fallback failed for %s (rc=%d ext=%d msg=%s)",
                  db_fullpath.c_str(), errorCode, memExt, memMsg ? memMsg : "unknown");
      }
    }
#endif
    if (create && errorCode == SQLITE_CANTOPEN)
    {
      CLog::Log(LOGFATAL, "SqliteDatabase: can't open %s", db_fullpath.c_str());
      throw std::runtime_error("SqliteDatabase: can't open " + db_fullpath);
    }
    else if (errorCode == SQLITE_OK)
    {
      sqlite3_extended_result_codes(conn, 1);
      sqlite3_busy_handler(conn, busy_callback, NULL);
      char* err=NULL;
      if (setErr(sqlite3_exec(getHandle(),"PRAGMA empty_result_callbacks=ON",NULL,NULL,&err),"PRAGMA empty_result_callbacks=ON") != SQLITE_OK)
      {
        throw DbErrors("%s", getErrorMsg());
      }
      else if (sqlite3_db_readonly(conn, nullptr) == 1)
      {
        CLog::Log(LOGFATAL, "SqliteDatabase: %s is read only", db_fullpath.c_str());
        throw std::runtime_error("SqliteDatabase: " + db_fullpath + " is read only");
      }
      active = true;
      return DB_CONNECTION_OK;
    }

    return DB_CONNECTION_NONE;
  }
  catch(const DbErrors&)
  {
  }
  return DB_CONNECTION_NONE;
}

bool SqliteDatabase::exists(void)
{
  bool bRet = false;
  if (!active) return bRet;
  result_set res;
  char sqlcmd[512];

  // performing a select all on the sqlite_master will return rows if there are tables
  // defined indicating it's not empty and therefore must "exist".
  sprintf(sqlcmd,"SELECT * FROM sqlite_master");
  if ((last_err = sqlite3_exec(getHandle(),sqlcmd, &callback, &res,NULL)) == SQLITE_OK)
  {
    bRet = (res.records.size() > 0);
  }

  return bRet;
}

void SqliteDatabase::disconnect(void) {
  if (active == false) return;
  sqlite3_close(conn);
  active = false;
}

int SqliteDatabase::create() {
  return connect(true);
}

int SqliteDatabase::copy(const char *backup_name) {
  if (active == false)
    throw DbErrors("Can't copy database: no active connection...");

  CLog::Log(LOGDEBUG, "Copying from %s to %s at %s", db.c_str(), backup_name, host.c_str());

  int rc;
  std::string backup_db = backup_name;

  sqlite3 *pFile;           /* Database connection opened on zFilename */
  sqlite3_backup *pBackup;  /* Backup object used to copy data */

  //
  if (backup_name[0] == '/' || backup_name[0] == '\\')
    backup_db = backup_db.substr(1);

  // ensure the ".db" extension is appended to the end
  if ( backup_db.find(".db") != (backup_db.length()-3) )
    backup_db += ".db";

  std::string backup_path = host + backup_db;

  /* Open the database file identified by zFilename. Exit early if this fails
  ** for any reason. */
  rc = sqlite3_open(backup_path.c_str(), &pFile);
  if( rc==SQLITE_OK )
  {
    pBackup = sqlite3_backup_init(pFile, "main", getHandle(), "main");

    if( pBackup )
    {
      (void)sqlite3_backup_step(pBackup, -1);
      (void)sqlite3_backup_finish(pBackup);
    }

    rc = sqlite3_errcode(pFile);
  }

  (void)sqlite3_close(pFile);

  if( rc != SQLITE_OK )
    throw DbErrors("Can't copy database. (%d)", rc);

  return rc;
}

int SqliteDatabase::drop_analytics(void) {
  // SqliteDatabase::copy used a full database copy, so we have a new version
  // with all the analytics stuff. We should clean database from everything but data
  if (active == false)
    throw DbErrors("Can't drop extras database: no active connection...");

  char sqlcmd[4096];
  result_set res;

  CLog::Log(LOGDEBUG, "Cleaning indexes from database %s at %s", db.c_str(), host.c_str());
  sprintf(sqlcmd, "SELECT name FROM sqlite_master WHERE type == 'index' AND sql IS NOT NULL");
  if ((last_err = sqlite3_exec(conn, sqlcmd, &callback, &res, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;

  for (size_t i=0; i < res.records.size(); i++) {
    sprintf(sqlcmd,"DROP INDEX '%s'", res.records[i]->at(0).get_asString().c_str());
    if ((last_err = sqlite3_exec(conn, sqlcmd, NULL, NULL, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
  }
  res.clear();

  CLog::Log(LOGDEBUG, "Cleaning views from database %s at %s", db.c_str(), host.c_str());
  sprintf(sqlcmd, "SELECT name FROM sqlite_master WHERE type == 'view'");
  if ((last_err = sqlite3_exec(conn, sqlcmd, &callback, &res, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;

  for (size_t i=0; i < res.records.size(); i++) {
    sprintf(sqlcmd,"DROP VIEW '%s'", res.records[i]->at(0).get_asString().c_str());
    if ((last_err = sqlite3_exec(conn, sqlcmd, NULL, NULL, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
  }
  res.clear();

  CLog::Log(LOGDEBUG, "Cleaning triggers from database %s at %s", db.c_str(), host.c_str());
  sprintf(sqlcmd, "SELECT name FROM sqlite_master WHERE type == 'trigger'");
  if ((last_err = sqlite3_exec(conn, sqlcmd, &callback, &res, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;

  for (size_t i=0; i < res.records.size(); i++) {
    sprintf(sqlcmd,"DROP TRIGGER '%s'", res.records[i]->at(0).get_asString().c_str());
    if ((last_err = sqlite3_exec(conn, sqlcmd, NULL, NULL, NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
  }
  // res would be cleared on destruct

  return DB_COMMAND_OK;
}

int SqliteDatabase::drop() {
  if (active == false) throw DbErrors("Can't drop database: no active connection...");
  disconnect();
  if (!unlink(db.c_str())) {
     throw DbErrors("Can't drop database: can't unlink the file %s,\nError: %s",db.c_str(),strerror(errno));
     }
  return DB_COMMAND_OK;
}


long SqliteDatabase::nextid(const char* sname) {
  if (!active) return DB_UNEXPECTED_RESULT;
  int id;/*,nrow,ncol;*/
  result_set res;
  char sqlcmd[512];
  sprintf(sqlcmd,"select nextid from %s where seq_name = '%s'",sequence_table.c_str(), sname);
  if ((last_err = sqlite3_exec(getHandle(),sqlcmd,&callback,&res,NULL)) != SQLITE_OK) {
    return DB_UNEXPECTED_RESULT;
    }
  if (res.records.empty()) {
    id = 1;
    sprintf(sqlcmd,"insert into %s (nextid,seq_name) values (%d,'%s')",sequence_table.c_str(),id,sname);
    if ((last_err = sqlite3_exec(conn,sqlcmd,NULL,NULL,NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;
  }
  else {
    id = res.records[0]->at(0).get_asInt()+1;
    sprintf(sqlcmd,"update %s set nextid=%d where seq_name = '%s'",sequence_table.c_str(),id,sname);
    if ((last_err = sqlite3_exec(conn,sqlcmd,NULL,NULL,NULL)) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;
  }
  return DB_UNEXPECTED_RESULT;
}


// methods for transactions
// ---------------------------------------------
void SqliteDatabase::start_transaction() {
  if (active) {
    sqlite3_exec(conn,"begin IMMEDIATE",NULL,NULL,NULL);
    _in_transaction = true;
  }
}

void SqliteDatabase::commit_transaction() {
  if (active) {
    sqlite3_exec(conn,"commit",NULL,NULL,NULL);
    _in_transaction = false;
  }
}

void SqliteDatabase::rollback_transaction() {
  if (active) {
    sqlite3_exec(conn,"rollback",NULL,NULL,NULL);
    _in_transaction = false;
  }
}


// methods for formatting
// ---------------------------------------------
std::string SqliteDatabase::vprepare(const char *format, va_list args)
{
  std::string strFormat = format;
  std::string strResult = "";
  char *p;
  size_t pos;

  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  pos = 0;
  while ( (pos = strFormat.find("%s", pos)) != std::string::npos )
    strFormat.replace(pos++, 2, "%q");

  //  the %I64 enhancement is not supported by sqlite3_vmprintf
  //  must be %ll instead
  pos = 0;
  while ( (pos = strFormat.find("%I64", pos)) != std::string::npos )
    strFormat.replace(pos++, 4, "%ll");

  p = sqlite3_vmprintf(strFormat.c_str(), args);
  if ( p )
  {
    strResult = p;
    sqlite3_free(p);
  }

  // Strip SEPARATOR from all GROUP_CONCAT statements:
  // before: GROUP_CONCAT(field SEPARATOR '; ')
  // after:  GROUP_CONCAT(field, '; ')
  pos = strResult.find("GROUP_CONCAT(");
  while (pos != std::string::npos)
  {
    size_t pos2 = strResult.find(" SEPARATOR ", pos + 1);
    if (pos2 != std::string::npos)
      strResult.replace(pos2, 10, ",");
    pos = strResult.find("GROUP_CONCAT(", pos + 1);
  }
  // Replace CONCAT with || to concatenate text fields:
  // before: CONCAT(field1, field2)
  // after:  field1 || field2
  pos = strResult.find("CONCAT(");
  while (pos != std::string::npos)
  {
    if (pos == 0 || strResult[pos - 1] == ' ') // Not GROUP_CONCAT
    {
      size_t pos2 = strResult.find(",", pos + 1);
      if (pos2 != std::string::npos)
      {
        size_t pos3 = strResult.find(")", pos2 + 1);
        if (pos3 != std::string::npos)
        {
          strResult.erase(pos3, 1);
          strResult.replace(pos2, 1, " || ");
          strResult.erase(pos, 7);
        }
      }
    }
    pos = strResult.find("CONCAT(", pos + 1);
  }

  return strResult;
}


//************* SqliteDataset implementation ***************

SqliteDataset::SqliteDataset():Dataset() {
  haveError = false;
  db = NULL;
  errmsg = NULL;
  autorefresh = false;
}


SqliteDataset::SqliteDataset(SqliteDatabase *newDb):Dataset(newDb) {
  haveError = false;
  db = newDb;
  errmsg = NULL;
  autorefresh = false;
}

 SqliteDataset::~SqliteDataset(){
   if (errmsg) sqlite3_free(errmsg);
 }


void SqliteDataset::set_autorefresh(bool val){
    autorefresh = val;
}



//--------- protected functions implementation -----------------//

sqlite3* SqliteDataset::handle(){
  if (db != NULL){
    return static_cast<SqliteDatabase*>(db)->getHandle();
      }
  else return NULL;
}

void SqliteDataset::make_query(StringList &_sql) {
  std::string query;
  if (db == NULL) throw DbErrors("No Database Connection");

 try {

  if (autocommit) db->start_transaction();


  for (std::list<std::string>::iterator i =_sql.begin(); i!=_sql.end(); ++i) {
  query = *i;
  char* err=NULL;
  Dataset::parse_sql(query);
  if (db->setErr(sqlite3_exec(this->handle(),query.c_str(),NULL,NULL,&err),query.c_str())!=SQLITE_OK) {
    std::string message = db->getErrorMsg();
    if (err) {
      message.append(" (");
      message.append(err);
      message.append(")");
      sqlite3_free(err);
    }
    throw DbErrors("%s", message.c_str());
  }
  } // end of for


  if (db->in_transaction() && autocommit) db->commit_transaction();

  active = true;
  ds_state = dsSelect;
  if (autorefresh)
    refresh();

 } // end of try
 catch(...) {
  if (db->in_transaction()) db->rollback_transaction();
  throw;
 }

}


void SqliteDataset::make_insert() {
  make_query(insert_sql);
  last();
}


void SqliteDataset::make_edit() {
  make_query(update_sql);
}


void SqliteDataset::make_deletion() {
  make_query(delete_sql);
}


void SqliteDataset::fill_fields() {
  //cout <<"rr "<<result.records.size()<<"|" << frecno <<"\n";
  if ((db == NULL) || (result.record_header.empty()) || (result.records.size() < (unsigned int)frecno)) return;

  if (fields_object->size() == 0) // Filling columns name
  {
    const unsigned int ncols = result.record_header.size();
    fields_object->resize(ncols);
    for (unsigned int i = 0; i < ncols; i++)
      (*fields_object)[i].props = result.record_header[i];
  }

  //Filling result
  if (result.records.size() != 0)
  {
    const sql_record *row = result.records[frecno];
    if (row)
    {
      const unsigned int ncols = row->size();
      fields_object->resize(ncols);
      for (unsigned int i = 0; i < ncols; i++)
        (*fields_object)[i].val = row->at(i);
      return;
    }
  }
  const unsigned int ncols = result.record_header.size();
  fields_object->resize(ncols);
  for (unsigned int i = 0; i < ncols; i++)
    (*fields_object)[i].val = "";
}


//------------- public functions implementation -----------------//
bool SqliteDataset::dropIndex(const char *table, const char *index)
{
  std::string sql;

  sql = static_cast<SqliteDatabase*>(db)->prepare("DROP INDEX IF EXISTS %s", index);

  return (exec(sql) == SQLITE_OK);
}


int SqliteDataset::exec(const std::string &sql) {
  if (!handle()) throw DbErrors("No Database Connection");
  std::string qry = sql;
  int res;
  exec_res.clear();

  // Strip size constraints from indexes (not supported in sqlite)
  //
  // Example:
  //   before: CREATE UNIQUE INDEX ixPath ON path ( strPath(255) )
  //   after:  CREATE UNIQUE INDEX ixPath ON path ( strPath )
  //
  // NOTE: unexpected results occur if brackets are not matched
  if ( qry.find("CREATE UNIQUE INDEX") != std::string::npos ||
      (qry.find("CREATE INDEX") != std::string::npos))
  {
    size_t pos = 0;
    size_t pos2 = 0;

    if ( (pos = qry.find("(")) != std::string::npos )
    {
      pos++;
      while ( (pos = qry.find("(", pos)) != std::string::npos )
      {
        if ( (pos2 = qry.find(")", pos)) != std::string::npos )
        {
          qry.replace(pos, pos2-pos+1, "");
          pos = pos2;
        }
      }
    }
  }
  // Strip ON table from DROP INDEX statements:
  // before: DROP INDEX foo ON table
  // after:  DROP INDEX foo
  size_t pos = qry.find("DROP INDEX ");
  if ( pos != std::string::npos )
  {
    pos = qry.find(" ON ", pos+1);

    if ( pos != std::string::npos )
      qry = qry.substr(0, pos);
  }

  if((res = db->setErr(sqlite3_exec(handle(),qry.c_str(),&callback,&exec_res,&errmsg),qry.c_str())) == SQLITE_OK)
    return res;
  else
    {
      if (errmsg)
        throw DbErrors("%s (%s)", db->getErrorMsg(), errmsg);
      else
        throw DbErrors("%s", db->getErrorMsg());
    }
}

int SqliteDataset::exec() {
  return exec(sql);
}

const void* SqliteDataset::getExecRes() {
  return &exec_res;
}


bool SqliteDataset::query(const std::string &query) {
    if(!handle()) throw DbErrors("No Database Connection");
    std::string qry = query;
    int fs = qry.find("select");
    int fS = qry.find("SELECT");
    if (!( fs >= 0 || fS >=0))
         throw DbErrors("MUST be select SQL!");

  close();

  sqlite3_stmt *stmt = NULL;
  if (db->setErr(sqlite3_prepare_v2(handle(),query.c_str(),-1,&stmt, NULL),query.c_str()) != SQLITE_OK)
    throw DbErrors("%s", db->getErrorMsg());

  // column headers
  const unsigned int numColumns = sqlite3_column_count(stmt);
  result.record_header.resize(numColumns);
  for (unsigned int i = 0; i < numColumns; i++)
    result.record_header[i].name = sqlite3_column_name(stmt, i);

  // returned rows
  while (sqlite3_step(stmt) == SQLITE_ROW)
  { // have a row of data
    sql_record *res = new sql_record;
    res->resize(numColumns);
    for (unsigned int i = 0; i < numColumns; i++)
    {
      field_value &v = res->at(i);
      switch (sqlite3_column_type(stmt, i))
      {
      case SQLITE_INTEGER:
        v.set_asInt64(sqlite3_column_int64(stmt, i));
        break;
      case SQLITE_FLOAT:
        v.set_asDouble(sqlite3_column_double(stmt, i));
        break;
      case SQLITE_TEXT:
        v.set_asString((const char *)sqlite3_column_text(stmt, i));
        break;
      case SQLITE_BLOB:
        v.set_asString((const char *)sqlite3_column_text(stmt, i));
        break;
      case SQLITE_NULL:
      default:
        v.set_asString("");
        v.set_isNull();
        break;
      }
    }
    result.records.push_back(res);
  }
  if (db->setErr(sqlite3_finalize(stmt),query.c_str()) == SQLITE_OK)
  {
    active = true;
    ds_state = dsSelect;
    this->first();
    return true;
  }
  else
  {
    throw DbErrors("%s", db->getErrorMsg());
  }
}

void SqliteDataset::open(const std::string &sql) {
  set_select_sql(sql);
  open();
}

void SqliteDataset::open() {
  if (select_sql.size()) {
    query(select_sql);
  }
  else {
    ds_state = dsInactive;
  }
}


void SqliteDataset::close() {
  Dataset::close();
  result.clear();
  edit_object->clear();
  fields_object->clear();
  ds_state = dsInactive;
  active = false;
}


void SqliteDataset::cancel() {
  if ((ds_state == dsInsert) || (ds_state==dsEdit)) {
    if (result.record_header.size())
      ds_state = dsSelect;
    else
      ds_state = dsInactive;
  }
}


int SqliteDataset::num_rows() {
  return result.records.size();
}


bool SqliteDataset::eof() {
  return feof;
}


bool SqliteDataset::bof() {
  return fbof;
}


void SqliteDataset::first() {
  Dataset::first();
  this->fill_fields();
}

void SqliteDataset::last() {
  Dataset::last();
  fill_fields();
}

void SqliteDataset::prev(void) {
  Dataset::prev();
  fill_fields();
}

void SqliteDataset::next(void) {
  Dataset::next();
  if (!eof())
      fill_fields();
}

void SqliteDataset::free_row(void)
{
  if (frecno < 0 || (unsigned int)frecno >= result.records.size())
    return;

  sql_record *row = result.records[frecno];
  if (row)
  {
    delete row;
    result.records[frecno] = NULL;
  }
}

bool SqliteDataset::seek(int pos) {
  if (ds_state == dsSelect) {
    Dataset::seek(pos);
    fill_fields();
    return true;
    }
  return false;
}

int64_t SqliteDataset::lastinsertid()
{
  if(!handle()) throw DbErrors("No Database Connection");
  return sqlite3_last_insert_rowid(handle());
}


long SqliteDataset::nextid(const char *seq_name) {
  if (handle()) return db->nextid(seq_name);
  else return DB_UNEXPECTED_RESULT;
}

void SqliteDataset::interrupt() {
  sqlite3_interrupt(handle());
}
}//namespace
