#include <string>
#include <iostream>
#include <fstream>
//#include <boost/program_options.hpp>
#include <boost/config.hpp>
//#include <boost/scoped_ptr.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>
//#include <mongo/client/dbclient.h>
#include <mongo/client/connpool.h>
#include "sipdb/MongoDB.h"
#include <os/OsLogger.h>

using namespace MongoDB;
using namespace std;

namespace pod = boost::program_options::detail;

BaseDB::BaseDB(const ConnectionInfo& info) :
            _info(info)
{  
}

bool ConnectionInfo::testConnection(const mongo::ConnectionString &connectionString, string& errmsg)
{
    bool ret = false;

    try
    {
        MongoDB::ScopedDbConnectionPtr conn(mongoMod::ScopedDbConnection::getScopedDbConnection(connectionString.toString()));
        ret = conn->ok();
        conn->done();
    }
    catch( mongo::DBException& e )
    {
        ret = false;
        errmsg = e.what();
    }

    return ret;
}

const ConnectionInfo ConnectionInfo::globalInfo()
{
	const char* fname = SIPX_CONFDIR "/mongo-client.ini";
	ifstream file(fname);
    if (!file)
    {
        BOOST_THROW_EXCEPTION(ConfigError() <<  errmsg_info(std::string("Missing file ")  + fname));
    }
    return ConnectionInfo(file);
}

const ConnectionInfo ConnectionInfo::localInfo()
{
  ifstream file(SIPX_CONFDIR "/mongo-local.ini");

  if (!file)
  {
    return ConnectionInfo();
  }

  return ConnectionInfo(file);
}

ConnectionInfo::ConnectionInfo(ifstream& file) :
    _shard(0),
    _useReadTags(false),
    _queryTimeoutMs(0)
{
  set<string> options;
  options.insert("*");
  string connectionString;
  for (boost::program_options::detail::config_file_iterator i(file, options), e; i != e; ++i)
  {
    if (i->string_key == "connectionString")
    {
      connectionString = i->value[0];
    }

    if (i->string_key == "shardId")
    {
      _shard = atoi(i->value[0].c_str());
    }

    if (i->string_key == "clusterId")
    {
      _clusterId = i->value[0];
    }

    if (i->string_key == "useReadTags")
    {
      Os::Logger::instance().log(FAC_SIP, PRI_DEBUG, i->value[0].c_str());
      if (strncmp(i->value[0].c_str(), "true", 4) == 0)
      {
        _useReadTags = true;
      }
    }

    if (i->string_key == "query-timeout-ms")
    {
      _queryTimeoutMs = atoi(i->value[0].c_str());
    }
  }

  OS_LOG_INFO(FAC_SIP, "ConnectionInfo::ConnectionInfo "
      << "connectionString: " << connectionString
      << ", shardId: " << _shard
      << ", clusterId: " << _clusterId
      << ", useReadTags: " << _useReadTags
      << ", queryTimeoutMs: " << _queryTimeoutMs);

  file.close();
  if (connectionString.size() == 0)
  {
    BOOST_THROW_EXCEPTION(ConfigError() << errmsg_info(std::string("Invalid contents, missing parameter 'connectionString' in file ")));
  }

  string errmsg;
  _connectionString = mongo::ConnectionString::parse(connectionString, errmsg);
  if (!_connectionString.isValid())
  {
    BOOST_THROW_EXCEPTION(ConfigError() << errmsg_info(errmsg));
  }

  Os::Logger::instance().log(FAC_SIP, PRI_DEBUG, "loaded db connection info for %s", connectionString.c_str());
}

void  BaseDB::setReadPreference(mongo::BSONObjBuilder& builder, mongo::BSONObj query, const char* readPreferrence) const
{
  if (_info.useReadTags())
  {
    Os::Logger::instance().log(FAC_SIP, PRI_DEBUG, "Using read preferences tags for ");
    std::string shardIdStr = boost::to_string(getShardId());
    std::string clusterId = getClusterId();

    if (clusterId.empty())
    {
      clusterId = "1"; // for backward compatibility with old behavior
    }

    mongo::BSONArray tags = BSON_ARRAY(BSON("clusterId" << clusterId) << BSON("shardId" << shardIdStr));
    builder.append("$readPreference", BSON("mode" << readPreferrence << "tags" << tags));
  }
  else
  {
    builder.append("$readPreference", BSON("mode" << readPreferrence));
  }

  builder.append("query", query);
}

void  BaseDB::nearest(mongo::BSONObjBuilder& builder, mongo::BSONObj query) const
{
  setReadPreference(builder, query, "nearest");
}
	
void  BaseDB::primaryPreferred(mongo::BSONObjBuilder& builder, mongo::BSONObj query) const
{
  setReadPreference(builder, query, "primaryPreferred");
}

void BaseDB::forEach(mongo::BSONObj& query, const std::string& ns, boost::function<void(mongo::BSONObj)> doSomething)
{
  MongoDB::ScopedDbConnectionPtr conn(mongoMod::ScopedDbConnection::getScopedDbConnection(_info.getConnectionString().toString(), getQueryTimeout()));
  auto_ptr<mongo::DBClientCursor> pCursor = conn->get()->query(ns, query, 0, 0, 0, mongo::QueryOption_SlaveOk);
  if (pCursor.get() && pCursor->more())
  {
    while (pCursor->more())
    {
      doSomething(pCursor->next());
    }
  }

  conn->done();
}

