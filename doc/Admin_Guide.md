LDP Administrator Guide
=======================

##### Contents  
1\. Overview  
2\. System requirements  
3\. Installation  
4\. Database configuration  
5\. Server configuration  
6\. Direct extraction  
7\. Data privacy  
Reference


1\. Overview
------------

An _LDP instance_ is composed of a running LDP server and a database.
The LDP server updates data in the database from data sources such as
FOLIO modules, and users connect directly to the database to perform
reporting and analytics.

LDP is not multitenant in the usual sense, and normally one LDP
instance is deployed per library.  However, shared data from multiple
libraries of a consortium can be stored in a single LDP instance, and
in that case we refer to each of the libraries as a tenant.

This administrator guide covers installation and configuration of an
LDP instance.


2\. System requirements
-----------------------

### Software

* Operating systems supported:
  * Linux 4.18.0 or later
  * macOS 10.15.3 or later (non-production use only)
* Database systems supported:
  * [PostgreSQL](https://www.postgresql.org/) 11 or later
  * [Amazon Redshift](https://aws.amazon.com/redshift/) 1.0.12094 or later
* Other software dependencies:
  * ODBC driver for [PostgreSQL](https://odbc.postgresql.org/) or [Redshift](https://docs.aws.amazon.com/redshift/latest/mgmt/configure-odbc-connection.html#install-odbc-driver-linux)
  * [unixODBC](http://www.unixodbc.org/) 2.3.4 or later
  * [libpq](https://www.postgresql.org/) 11.5 or later
  * [libcurl](https://curl.haxx.se/) 7.64.0 or later
  * [RapidJSON](https://rapidjson.org/) 1.1.0 or later
* Required to build from source code:
  * C++ compilers supported:
    * [GCC C++ compiler](https://gcc.gnu.org/) 8.3.0 or later
    * [Clang](https://clang.llvm.org/) 8.0.1 or later
  * [CMake](https://cmake.org/) 3.16.2 or later

### Hardware

The LDP software and database are designed to be performant on low
cost hardware, and in most cases they should run well with the
following minimum requirements:

* Database:
  * Memory: 1 GB
  * Storage: 500 GB HDD
* LDP software:
  * Memory: 100 MB
  * Storage: 160 GB HDD

For large libraries, or if very high performance is desired, the
database CPU and memory can be increased as needed.  Alternatively,
Amazon Redshift can be used instead of PostgreSQL for significantly
higher performance and scalability.  The database storage capacity
also can be increased as needed.


3\. Installation
----------------

### Releases and branches

LDP releases use version numbers in the form, _a_._b_._c_, where
_a_._b_ is the major version and _c_ > 0 indicates a bug fix version.
Suppose that LDP 1.3.0 has been released.  Any subsequent versions
with the same major version number (1.3), for example, 1.3.1 or 1.3.2,
will not generally contain new features but only bug fixes.

The most recent major release and related bug fix releases are
available from the [releases
page](https://github.com/library-data-platform/ldp/releases).

More options are available within the [source code
repository](https://github.com/library-data-platform/ldp), which
contains two main kinds of branches:

* Release branches (`*-release`):  Release branches are the "stable"
  branches.  They are the most stable versions available because they
  generally receive only bug fixes and no new features.  A release
  branch such as `1.2-release` would point to the latest version of
  LDP 1.2, for example, 1.2.5.  It is recommended for production
  environments, or any environment where stability is critical, that
  they use only release branches.

* Current branch (`*-current`):  The current branch is a "development"
  branch.  It is the most stable of the development branches, in the
  sense that it has had some testing, but it is less stable than a
  release branch.  The advantage of using the current branch is that
  it contains new features long before those features are available in
  a release branch.  At any time there is only one current branch,
  which is named for the next planned release, and it eventually comes
  to be a release branch.  For example, `1.3-current` becomes
  `1.3-release` once it has stabilized.  If there is a need for faster
  access to new features than what is provided by release branches,
  the current branch may be used, but with the caveat that it could be
  unstable.

Other branches having various names are used for development such as
prototyping new features or bug fixes.

An LDP instance based on a release branch can be "upgraded" to the
current branch, but not generally from the current branch to a release
branch.  However, as explained above, the current branch eventually
becomes the next release branch.

### Installing software dependencies

Dependencies required for building the LDP software can be installed via
a package manager on some platforms.

#### Debian Linux

```shell
$ sudo apt update
$ sudo apt install cmake g++ libcurl4-openssl-dev libpq-dev \
      postgresql-server-dev-all rapidjson-dev unixodbc unixodbc-dev
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ sudo apt install odbc-postgresql
```

#### RHEL/CentOS Linux

```shell
$ sudo dnf install cmake gcc-c++ libcurl-devel libpq-devel make \
      postgresql-server-devel unixODBC-devel
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ sudo dnf install postgresql-odbc
```

RapidJSON can be [installed from
source](https://rapidjson.org/index.html#autotoc_md5).

#### macOS

Using [Homebrew](https://brew.sh/):

```shell
$ brew install cmake postgresql psqlodbc rapidjson unixodbc
```

For PostgreSQL, the ODBC driver can be installed with:

```shell
$ brew install psqlodbc
```

### Building the software

If the LDP software was built previously in the same directory, first
remove the leftover `build/` subdirectory to ensure a clean compile.
Then:

```shell
$ ./all.sh
```

The `all.sh` script creates a `build/` subdirectory and builds the
`ldp` executable there:

```shell
$ ./build/ldp help
```


4\. Database configuration
--------------------------

### Creating a database

Before using the LDP software, we have to create a database that will
store the data.  This can be a local or cloud-based PostgreSQL
database or a cloud-based Redshift database.

A robust backup process should be used to ensure that historical data
and local tables are safe.

#### Large data and PostgreSQL

PostgreSQL is not optimized for analytic queries.  Users with large
data that wish to use PostgreSQL might consider installing
[cstore_fdw](https://github.com/citusdata/cstore_fdw), an open source
column storage extension which enables very fast analytic queries on
large tables.  Although LDP does not currently generate columnar
tables with cstore_fdw, it may be helpful for users with large,
frequently queried data to copy them into columnar tables.  However,
please note that prior to PostgreSQL 13, `pg_dump` does not include
these columnar data in backups; so the original tables should be
retained as well.

Amazon/AWS Relational Database Service (RDS) does not currently
support cstore_fdw.  In order to use cstore_fdw in AWS, PostgreSQL can
be installed on an EC2 instance, although this is a more manual
process than deploying PostgreSQL in RDS.

Recent versions of Debian Linux support installing cstore_fdw via the
`apt` package manager.

#### PostgreSQL

For libraries that deploy LDP with PostgreSQL, whether local or
hosted, we recommend setting:

* `checkpoint_timeout`: `3600` (seconds)
* `max_wal_size`: `100000` (MB)
* `work_mem`: `64000` (KB), if sufficient memory is available

#### PostgreSQL hosted in RDS

For libraries that deploy LDP with cloud-based PostgreSQL using RDS,
we recommend setting:

* Instance type:  `db.m5.large`
* Number of instances:  `1`
* Storage:  `General Purpose SSD`
* Snapshots:  Automated snapshots enabled

#### Redshift

For libraries that deploy LDP with Redshift, we recommend setting:

* Node type:  `dc2.large`
* Cluster type:  `Multiple Node`
* Number of compute nodes:  `2`
* Snapshots:  Automated snapshots enabled
* Maintenance Track:  `Trailing`

### Configuring the database

Three database users are required:

* `ldpadmin` owns all database objects created by the LDP software.
  This account should be used very sparingly and carefully.

* `ldpconfig` is a special user account for changing configuration
  settings in the `dbconfig` schema.  It is intended to enable
  designated users to make changes to the server's operation, such as
  scheduling when data updates occur.  This user name can be modified
  using the `ldpconfig_user` configuration setting in `ldpconf.json`.

* `ldp` is a general user of the LDP database.  This user name can be
  modified using the `ldp_user` configuration setting in
  `ldpconf.json`.

If more than one LDP instance will be hosted with a single database
server, the `ldpconfig` and `ldp` user names should for security
reasons be configured to be different for each LDP instance.  This is
done by including within `ldpconf.json` the `ldpconfig_user` and
`ldp_user` settings described below in the "Reference" section of this
guide.  In the following examples we will assume that the default user
names are being used, but please substitute alternative names if you
have configured them.

In addition to creating these users, a few access permissions should
be set.  In PostgreSQL, this can be done on the command line, for
example:

```shell
$ createuser ldpadmin --username=<admin_user> --pwprompt
$ createuser ldpconfig --username=<admin_user> --pwprompt
$ createuser ldp --username=<admin_user> --pwprompt
$ createdb ldp --username=<admin_user> --owner=ldpadmin
$ psql ldp --username=<admin_user> --single-transaction \
      --command="REVOKE ALL ON SCHEMA public FROM public;" \
      --command="GRANT ALL ON SCHEMA public TO ldpadmin;" \
      --command="GRANT USAGE ON SCHEMA public TO ldpconfig;" \
      --command="GRANT USAGE ON SCHEMA public TO ldp;"
```

Additional command line options may be required to specify the
database host, port, etc.

In Redshift, this can be done in the database once it has been
created, for example:

```sql
CREATE USER ldpadmin PASSWORD '(ldpadmin password here)';
CREATE USER ldpconfig PASSWORD '(ldpconfig password here)';
CREATE USER ldp PASSWORD '(ldp password here)';
ALTER DATABASE ldp OWNER TO ldpadmin;
REVOKE ALL ON SCHEMA public FROM public;
GRANT ALL ON SCHEMA public TO ldpadmin;
GRANT USAGE ON SCHEMA public TO ldpconfig;
GRANT USAGE ON SCHEMA public TO ldp;
```

### Configuring ODBC

The LDP software uses unixODBC to connect to the LDP database.  To
configure the connection, install the ODBC driver for the database
system being used (PostgreSQL or Redshift), and create the files
`$HOME/.odbcinst.ini` and `$HOME/.odbc.ini`.  The provided example
files
[odbcinst.ini](https://raw.githubusercontent.com/library-data-platform/ldp/master/examples/odbcinst.ini)
and
[odbc.ini](https://raw.githubusercontent.com/library-data-platform/ldp/master/examples/odbc.ini)
can be used as templates.

__odbcinst.ini__
```
[PostgreSQL]
Description = PostgreSQL
Driver = /usr/lib/x86_64-linux-gnu/odbc/psqlodbcw.so
FileUsage = 1
```

__odbc.ini__
```
[ldp]
Description = ldp
Driver = PostgreSQL
Database = ldp
Servername = ldp.indexdata.com
UserName = ldpadmin
Password = (ldpadmin password here)
Port = 5432
SSLMode = require
```

The command-line tool `isql` is included with unixODBC and can be used
to test the connection:

```
$ isql ldp
+---------------------------------------+
| Connected!                            |
|                                       |
| sql-statement                         |
| help [tablename]                      |
| quit                                  |
|                                       |
+---------------------------------------+
SQL>
```


5\. Server configuration
------------------------

### Creating a data directory

LDP uses a "data directory" where cached and temporary data, as well
as server configuration files, are stored.  In these examples, we will
suppose that the data directory is `/var/lib/ldp` and that the server
will be run as an `ldp` user:

```shell
$ sudo mkdir -p /var/lib/ldp
$ sudo chown ldp /var/lib/ldp
```

### Configuration file

Server configuration settings are stored in a file in the data
directory called `ldpconf.json`.  In our example it would be
`/var/lib/ldp/ldpconf.json`.  The provided example file
[ldpconf.json](https://raw.githubusercontent.com/library-data-platform/ldp/master/examples/ldpconf.json)
can be used as a template.

__ldpconf.json__
```
{
    "deployment_environment": "production",
    "ldp_database": {
        "odbc_database": "ldp"
    },
    "enable_sources": ["my_library"],
    "sources": {
        "my_library": {
            "okapi_url": "https://folio-snapshot-okapi.dev.folio.org",
            "okapi_tenant": "diku",
            "okapi_user": "diku_admin",
            "okapi_password": "(okapi password here)"
        }
    }
}
```

### Starting the server

If this is a new database, it should first be initialized:

```shell
$ ldp init-database -D /var/lib/ldp --profile folio
```

To start the LDP server:

```shell
$ nohup ldp server -D /var/lib/ldp &>> logfile &
```

The server logs details of its activities in the table
`dbsystem.log`.  For more detailed logging, the `--trace` option can
be used:

```shell
$ nohup ldp server -D /var/lib/ldp --trace &>> logfile &
```

Once per day, the server runs a _full update_ which performs all
supported data updates.  Full updates can be scheduled at a preferred
time of day using the table `dbconfig.general`.  See the
[Configuration Guide](Config_Guide.md) for more information.

### Updating data without the server

As an alternative to running the server, data updates can be performed
on an initialized database via the command line:

```shell
$ ldp update -D /var/lib/ldp
```

This will run a full update, showing progress on the console, and then
exit.  It can be scheduled via
[cron](https://en.wikipedia.org/wiki/Cron) to run once per day.

Note that `ldp server` and `ldp update` will not run at the same time.

### Upgrading to a new version

When installing a new version of LDP, the database should be
"upgraded" before starting the new server:

1. First, confirm that the new version of LDP builds without errors in
   the installation environment.

2. Make a backup of the database.

3. Stop the old version of the server.

4. Use the `upgrade-database` command in the new version of LDP to
   perform the upgrade, e.g.:

```shell
$ ldp upgrade-database -D /var/lib/ldp
```

5. Start the new version of the server.

Do not interrupt the database upgrade process in step 4.  Some schema
changes use DDL statements that cannot be run within a transaction,
and interrupting them may leave the database in an intermediate state.
For diagnostic purposes, database statements used to perform the
upgrade are logged to files located in the data directory under
`database_upgrade/`.

In automated deployments, the `upgrade-database` command can be run
after `git pull`, whether or not any new changes were pulled.  If no
upgrade is needed, it will exit normally:

```shell
$ ldp upgrade-database -D /var/lib/ldp ; echo $?
```
```shell
ldp: Database version is up to date
0
```


6\. Direct extraction
---------------------

LDP extracts most data via module APIs; but in some cases it is
necessary to extract directly from a module's internal database, such
as when the data are too large for the API to process.  In LDP this is
referred to as _direct extraction_ and is currently supported for the
following tables:

* `inventory_holdings`
* `inventory_instances`
* `inventory_items`

Direct extraction can be enabled by adding the list of tables and
database connection parameters to a source configuration, as in this
example:

```
{

    ( . . . )

    "sources": {
        "my_library": {

            ( . . . )

	    "direct_tables": [
	        "inventory_holdings",
		"inventory_instances",
		"inventory_items"
            ],
            "direct_database_name": "okapi",
            "direct_database_host": "database.indexdata.com",
            "direct_database_port": 5432,
            "direct_database_user": "folio_admin",
            "direct_database_password": "(database password here)"
        }
    }
}
```

Note that direct extraction requires network access to the database,
which may be protected by a firewall.


7\. Data privacy
----------------

LDP attempts to "anonymize" personal data.  This anonymization feature
is enabled unless otherwise configured.

Anonymization can be disabled by setting `anonymize` to `false` in
`ldpconf.json`.

__WARNING:  LDP does not provide a way to anonymize the database after
personal data have been loaded into it.  For this reason, these
settings should never be used unless you are absolutely sure that you
want to store personal data in the LDP database.__


Reference
---------

### Configuration file: ldpconf.json

* `deployment_environment` (string; required) is the deployment
  environment of the LDP instance.  Supported values are `production`,
  `staging`, `testing`, and `development`.  This setting is used to
  determine whether certain operations should be allowed to run on the
  instance.

* `ldp_database` (object; required) is a group of database-related
  settings.
  * `odbc_database` (string; required) is the ODBC "data source name"
    of the LDP database.
  * `ldpconfig_user` (string; optional) is the database user that is
    defined by default as `ldpconfig`.
  * `ldp_user` (string; optional) is the database user that is defined
    by default as `ldp`.

* `enable_sources` (array; required) is a list of sources that are
  enabled for LDP to extract data from.  The source names refer to a
  subset of those defined under `sources` (see below).  Only one
  source should be provided in the case of non-consortial deployments.

* `sources` (object; required) is a collection of sources that LDP can
  extract data from.  Only one source should be provided in the case
  of non-consortial deployments.  A source is defined by a source name
  and an associated object containing several settings:
  * `okapi_url` (string; required) is the URL for the Okapi instance
    to extract data from.
  * `okapi_tenant` (string; required) is the Okapi tenant.
  * `okapi_user` (string; required) is the Okapi user name.
  * `okapi_password` (string; required) is the password for the
    specified Okapi user name.
  * `tenant_id` (integer; optional) uniquely identifies a tenant in a
    consortial LDP deployment.  The default value is `1`.
  * `direct_tables` (array; optional) is a list of tables that should
    be updated using direct extraction.  Only these tables may be
    included: `inventory_holdings`, `inventory_instances`, and
    `inventory_items`.
  * `direct_database_name` (string; optional) is the FOLIO database
    name.
  * `direct_database_host` (string; optional) is the FOLIO database
    host name.
  * `direct_database_port` (integer; optional) is the FOLIO database
    port.
  * `direct_database_user` (string; optional) is the FOLIO database
    user name.
  * `direct_database_password` (string; optional) is the password for
    the specified FOLIO database user name.

* `anonymize` (Boolean; optional) when set to `false`, disables
  anonymization of personal data.  The default value is `true`.
  Please read the section on "Data privacy" above before changing this
  setting.

* `allow_destructive_tests` (Boolean; optional) when set to `true`,
  allows the LDP database to be overwritten by integration tests or
  other testing or development operations.  The default value is
  `false`.  This value should only be `true` for an LDP database that
  is being used as a testing sandbox, and never in a production or
  staging deployment.


Further reading
---------------

[__Learn about configuring LDP in the
Configuration Guide > > >__](Config_Guide.md)

[__Learn about using the LDP database in the
User Guide > > >__](User_Guide.md)

