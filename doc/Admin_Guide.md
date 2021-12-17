LDP Administrator Guide
=======================

##### Contents  
1\. [Overview](#1-overview)  
2\. [System requirements](#2-system-requirements)  
3\. [Installation](#3-installation)  
4\. [Database configuration](#4-database-configuration)  
5\. [LDP configuration](#5-ldp-configuration)  
6\. [Direct extraction](#6-direct-extraction)  
7\. [Data privacy](#7-data-privacy)  
8\. [Optional columns](#8-optional-columns)  
9\. [Historical data](#9-historical-data)  
10\. [User accounts](#10-user-accounts)  
[Reference](#reference)


1\. Overview
------------

An _LDP instance_ is composed of an LDP server configuration and an
analytic database.  The LDP server updates data in the database from
data sources such as FOLIO modules, and users connect directly to the
database to perform reporting and analytics.

LDP is not multitenant in the usual sense, and normally one LDP
instance is deployed per library.

This administrator guide covers installation and configuration of an
LDP instance.


2\. System requirements
-----------------------

### Software

* Operating systems supported:
  * Linux
* Database systems supported:
  * [PostgreSQL](https://www.postgresql.org/) 12.6 or later
* Other software dependencies:
  * [libpq](https://www.postgresql.org/) 12.6 or later
  * [libcurl](https://curl.haxx.se/) 7.64.0 or later
  * [RapidJSON](https://rapidjson.org/) 1.1.0 or later
* Required to build from source code:
  * [GCC C++ compiler](https://gcc.gnu.org/) 8.3.0 or later
  * [CMake](https://cmake.org/) 3.16.2 or later
* Required to build and run via Docker:
  * [Docker](https://docker.com) 17.05 or later


### Hardware

The LDP software and database are designed to be performant on low
cost hardware, and in most cases they should run well with the
following minimum requirements:

* LDP software:
  * Memory: 500 MB
  * Storage: 500 GB HDD
* Database:
  * Memory: 8 GB
  * Storage: 1 TB HDD

For higher performance, SSD drives are recommended for storage, and
the database CPU and memory should be increased.  The database storage
capacity also can be increased as needed.


3\. Installation
----------------

### Branches

The LDP repository has two types of branches:

* The main branch (`main`).  This is a development branch where new
  features are first merged.  This branch is relatively unstable.  It
  is also the default view when browsing the repository on GitHub.

* Release branches (`release-*`).  These are releases made from
  `main`.  They are managed as stable branches; i.e. they may receive
  bug fixes but generally no new features.  Most users should run a
  recent release branch.

### Installing software dependencies

Dependencies required for building the LDP software can be installed via
a package manager on some platforms.

#### Debian/Ubuntu Linux

```shell
$ sudo apt update
$ sudo apt install cmake g++ libcurl4-openssl-dev libpq-dev \
      postgresql-server-dev-all rapidjson-dev
```

#### RHEL/CentOS Linux

```shell
$ sudo dnf install cmake gcc-c++ libcurl-devel libpq-devel make \
      postgresql-server-devel
```

RapidJSON can be [installed from
source](https://rapidjson.org/index.html#autotoc_md5).

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

### Building the software via Docker

Pre-built container images are available in the Github Container
Registry, for example:

```shell
$ docker pull ghcr.io/library-data-platform/ldp:1.3.0
```

Or they can be built by cloning this repository locally and running:

```shell
$ docker build -t ldp:latest . 
```


4\. Database configuration
--------------------------

### Creating a database

Before using the LDP software, we have to create a database that will
store the data.  This can be a local or cloud-based PostgreSQL
database.

A robust backup process should be used to ensure that historical data
and local tables are safe.

#### PostgreSQL configuration

These are recommended configuration settings for the PostgreSQL server
that runs the LDP database:

* `checkpoint_timeout`: `3000` (s)
* `max_wal_size`: `10240` (MB)
* `idle_in_transaction_session_timeout`: `3600000` (ms)

### Configuring the database

Three database users are required:

* `ldpadmin` owns all database objects created by the LDP software.
  This account should be used very sparingly and carefully.

* `ldpconfig` is a special user account for changing configuration
  settings in the `dbconfig` schema.  It is intended to enable
  designated users to make changes to the server's operation.  This
  user name can be modified using the `ldpconfig_user` configuration
  setting in `ldpconf.json`.

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
$ psql ldp --username=<admin_user> \
      --command="ALTER DATABASE ldp SET search_path TO public;" \
      --command="REVOKE ALL ON SCHEMA public FROM public;" \
      --command="GRANT ALL ON SCHEMA public TO ldpadmin;" \
      --command="GRANT USAGE ON SCHEMA public TO ldpconfig;" \
      --command="GRANT USAGE ON SCHEMA public TO ldp;"
```

Or once the database has been created:

```sql
CREATE USER ldpadmin PASSWORD '(ldpadmin password here)';
CREATE USER ldpconfig PASSWORD '(ldpconfig password here)';
CREATE USER ldp PASSWORD '(ldp password here)';
ALTER DATABASE ldp OWNER TO ldpadmin;
ALTER DATABASE ldp SET search_path TO public;
REVOKE ALL ON SCHEMA public FROM public;
GRANT ALL ON SCHEMA public TO ldpadmin;
GRANT USAGE ON SCHEMA public TO ldpconfig;
GRANT USAGE ON SCHEMA public TO ldp;
```


5\. LDP configuration
---------------------

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
        "database_name": "ldp",
        "database_host": "ldp.folio.org",
        "database_port": 5432,
        "database_user": "ldpadmin",
        "database_password": "(ldpadmin password here)",
        "database_super_user": "postgres",
        "database_super_password": "(postgres password here)",
        "database_sslmode": "require"
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

### Running LDP

If this is a new database, it should first be initialized:

```shell
$ ldp init-database -D /var/lib/ldp
```

To start LDP:

```shell
$ ldp update -D /var/lib/ldp
```

This will run a full update, showing progress on the console, and then
exit.  It can be scheduled via
[cron](https://en.wikipedia.org/wiki/Cron) to run once per day.

The server logs details of its activities to standard error and in the
table `dbsystem.log`.  For more detailed logging to standard error,
the `--trace` option can be used.

### Upgrading to a new version

When installing a new version of LDP, the database should be
"upgraded" before starting the new server:

1. First, confirm that the new version of LDP builds without errors in
   the installation environment.

2. Make a backup of the database.

3. Use the `upgrade-database` command in the new version of LDP to
   perform the upgrade, e.g.:

```shell
$ ldp upgrade-database -D /var/lib/ldp
```

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

### Running LDP via Docker

LDP can be run as a Docker container similar to the process above,
except that the LDP data directory path (`-D`) should be omitted.
Instead, mount your local LDP data directory at `/var/lib/ldp`.
Examples:

```shell
$ docker run --rm -v /my/local/datadir:/var/lib/ldp ghcr.io/library-data-platform/ldp init-database
```
```shell
$ docker run --rm -v /my/local/datadir:/var/lib/ldp ghcr.io/library-data-platform/ldp update
```
```shell
$ docker run --rm -v /my/local/datadir:/var/lib/ldp ghcr.io/library-data-platform/ldp upgrade-database
```


6\. Direct extraction
---------------------

LDP currently extracts most data via module APIs; but in some cases it
is necessary to extract directly from a module's internal database,
such as when the data are too large for the API to process.  In LDP
this is referred to as _direct extraction_ and is currently supported
for the following tables:

* `inventory_holdings`
* `inventory_instances`
* `inventory_items`
* `po_receiving_history`
* `srs_marc`
* `srs_records`

The last of these, `srs_marc` and `srs_records`, are made available in
LDP only by direct extraction.  No historical data are retained for
`srs_marc` and `srs_records`.

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
		"inventory_items",
		"po_receiving_history",
		"srs_marc",
		"srs_records"
            ],
            "direct_database_name": "okapi",
            "direct_database_host": "database.folio.org",
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

### Anonymization

LDP attempts to "anonymize" tables or columns that contain personal
data.  This anonymization feature is enabled unless otherwise
configured.  Some tables are redacted entirely when anonymization is
enabled, including `audit_circulation_logs`, `configuration_entries`,
`notes`, and `user_users`.

Anonymization can be disabled by setting `anonymize` to `false` in
`ldpconf.json`.

__WARNING: LDP does not provide a way to anonymize the database after
personal data have been loaded into it.  For this reason,
anonymization should never be disabled unless you are absolutely sure
that you want to store personal data in the LDP database.__

### Filtering data fields

In addition or as an alternative to the pre-defined anonymization
described above, a filter can be used to drop specific JSON fields.
The filter is defined by creating a configuration file
`ldp_drop_field.conf` in the data directory.  If this file is present,
LDP will try to parse the JSON objects and remove the specified field
data during the update process.  Each line should provide the table
name and field path in the form:

```
<table> <field_path>
```

For example:

```
circulation_loans /userId
circulation_loans /proxyUserId
circulation_requests /requester/firstName
circulation_requests /requester/lastName
circulation_requests /requester/middleName
circulation_requests /requester/barcode
circulation_requests /requester/patronGroup
```


8\. Optional columns
--------------------

LDP creates table columns based on the presence of data in JSON
fields.  Sometimes a JSON field is optional or missing, and the
corresponding column in LDP is not created.  This can cause errors in
queries that otherwise may be valid and useful.

Optional or missing columns can be added in LDP by creating a
configuration file `ldp_add_column.conf` in the data directory.  If
this file is present, LDP will add the specified columns during the
update process.  Each line of the file should provide the table name,
column name, and data type in the form:

```
<table>.<column> <type>
```

For example:

```
inventory_instance_relationship_types.name varchar
po_purchase_orders.po_number varchar
po_purchase_orders.vendor varchar
```

Most columns have the data type `varchar(...)`, and this can be
written as `varchar` in the configuration file.  Other supported data
types include `bigint`, `boolean`, `numeric(12,2)`, and `timestamptz`.


9\. Historical data
-------------------

For all tables except `srs_marc` and `srs_records`, when LDP detects
that a record has changed since the last update, it retains the old
version of the record.  These historical data are stored in the
`history` schema.  This feature is enabled by default.

LDP can be configured not to record history, by setting
`record_history` to `false` in `ldpconf.json`.  If historical data
will not be needed, this can have the benefit of reducing the running
time of updates.


10\. User accounts
------------------

Individual user accounts can be created in PostgreSQL and used to
access LDP.  To enable LDP to set permissions for these users, the
`database_super_user` and `database_super_password` settings must be
defined in `ldpconf.json`.

The user names are configured by creating a configuration file
`ldp_users.conf` in the data directory.  Each line should contain one
user name, for example:

```
bifur
bofur
bombur
```

Then run the `update-users` command to set the permissions, for
example:

```shell
$ ldp update-users -D /var/lib/ldp
```

The `update-users` command should be run whenever a new user name is
added to `ldp_users.conf`.


Reference
---------

### Configuration file: ldpconf.json

* `anonymize` (Boolean; optional) when set to `false`, disables
  anonymization of personal data.  The default value is `true`.
  Please read the section on "Data privacy" above before changing this
  setting.

* `deployment_environment` (string; required) is the deployment
  environment of the LDP instance.  Supported values are `production`,
  `staging`, `testing`, and `development`.  This setting is used to
  determine whether certain operations should be allowed to run on the
  instance.

* `enable_sources` (array; required) is a list of sources that are
  enabled for LDP to extract data from.  The source names refer to a
  subset of those defined under `sources` (see below).  Only one
  source should be provided in the case of non-consortial deployments.

* `index_large_varchar` (Boolean; optional) when set to `true`,
  enables indexing of `varchar` text columns that have a length
  greater than 500.  The default is `false`.

* `ldp_database` (object; required) is a group of database-related
  settings.
  * `ldpconfig_user` (string; optional) is the database user that is
    defined by default as `ldpconfig`.
  * `ldp_user` (string; optional) is the database user that is defined
    by default as `ldp`.
  * `database_host` (string; required) is the LDP database host name.
  * `database_name` (string; required) is the LDP database name.
  * `database_password` (string; required) is the password for the
    specified LDP database administrator user name.
  * `database_port` (integer; required) is the LDP database port.
  * `database_sslmode` (string; required) is the LDP database
    connection SSL mode.
  * `database_super_user` (string; optional) is a superuser for the
    LDP database.  This is required to enable individual user accounts.
  * `database_super_password` (string; optional) is the password for
    the specified superuser (`database_super_user`).  This is required
    to enable individual user accounts.
  * `database_user` (string; required) is the LDP database
    administrator user name.

* `parallel_update` (Boolean; optional) when set to `false`, disables
  parallel updates.  The default value is `true`.  Disabling parallel
  updates can be useful to make debugging easier, but it will also
  slow down the update process.

* `parallel_vacuum` (Boolean; optional) when set to `false`, disables
  parallel vacuum in PostgreSQL 13 or later, which may slow down
  vacuuming but may be more friendly to concurrent user queries or
  other database operations.  The default value is `true`.  This
  setting should not be set to `false` with PostgreSQL 12.x or
  earlier.

* `record_history` (Boolean; optional) when set to `false`, disables
  recording historical data.  The default value is `true`.  Please
  read the section on "Historical data" above before changing this
  setting.

* `sources` (object; required) is a collection of sources that LDP can
  extract data from.  Only one source should be provided in the case
  of non-consortial deployments.  A source is defined by a source name
  and an associated object containing several settings:
  * `direct_database_host` (string; optional) is the FOLIO database
    host name.
  * `direct_database_name` (string; optional) is the FOLIO database
    name.
  * `direct_database_password` (string; optional) is the password for
    the specified FOLIO database user name.
  * `direct_database_port` (integer; optional) is the FOLIO database
    port.
  * `direct_database_user` (string; optional) is the FOLIO database
    user name.
  * `direct_tables` (array; optional) is a list of tables that should
    be updated using direct extraction.  Only these tables may be
    included: `inventory_holdings`, `inventory_instances`,
    `inventory_items`, `po_receiving_history`, `srs_marc`, and
    `srs_records`.
  * `okapi_password` (string; required) is the password for the
  * `okapi_tenant` (string; required) is the Okapi tenant.
  * `okapi_url` (string; required) is the URL for the Okapi instance
    to extract data from.
  * `okapi_user` (string; required) is the Okapi user name.
    specified Okapi user name.


Further reading
---------------

[__Configuration Guide__](Config_Guide.md)

[__User Guide__](User_Guide.md)

