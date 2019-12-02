LDP Admin Guide
===============

##### Contents  
1\. System requirements  
2\. Installing the software  
3\. Creating the LDP database  
4\. Configuration file  
5\. Loading data into the database  
6\. Running the data loader in production  
7\. Anonymization of personal data  
8\. Loading data from files (for testing only)  
9\. Disabling database TLS/SSL (for testing only)  
10\. Reporting bugs and feature requests


1\. System requirements
-----------------------

### Software

* Linux or FreeBSD
* [PostgreSQL](https://www.postgresql.org/) 11.5 or later; or
  [Amazon Redshift](https://aws.amazon.com/redshift/) 1.0.8995 or later
* To build the LDP software from source code:
  * [CMake](https://cmake.org/) 3.13.4 or later
  * [GCC C++ compiler](https://gcc.gnu.org/) 8.3.0 or later
  * [libpq](https://www.postgresql.org/) 11.5 or later
  * [libcurl](https://curl.haxx.se/) 7.64.0 or later
  * [RapidJSON](https://rapidjson.org/) 1.1.0 or later

### Hardware

The LDP is designed to be performant on low cost hardware, and in most
cases it should run well with the following minimum requirements:

* Database
  * Memory: 1 GB
  * Storage: 320 GB HDD
* LDP software (data loader)
  * Memory: 100 MB
  * Storage: 320 GB HDD

For large libraries, or if very high performance is desired, CPU and
memory can be increased as needed.  Alternatively, Redshift can be used
instead of PostgreSQL for significantly higher performance and
scalability.  The storage capacity also can be increased as needed.


2\. Installing the software
---------------------------

### Before installation

Dependencies required for building the LDP software can be installed via
a package manager on some platforms.

To install them in Debian 10.1:

```shell
$ sudo apt install cmake g++ libcurl4-openssl-dev libpq-dev \
      postgresql-server-dev-all rapidjson-dev
```

To install them in FreeBSD 12.1:

```shell
$ sudo pkg install cmake postgresql11-client rapidjson
```

The LDP software can be used on macOS for testing or development.  To
install the build dependencies using [Homebrew](https://brew.sh/):

```shell
$ brew install cmake postgresql rapidjson
```

### LDP software pre-releases

Before LDP 1.0, all LDP releases are "pre-releases" intended for testing
purposes.

Within the source code repository there are two main branches:

* `master` is the branch that releases are made from.  It contains
  recently added features that have had some testing.

* `current` is for active development and tends to be unstable.  This is
  where new features are first added.

<!--
### LDP software releases

Beginning with LDP 1.0, releases use version numbers in the form,
*a*.*b*.*c*, where *a*.*b* is the release version and *c* indicates bug
fixes.  For example, suppose that LDP 1.3 is released.  The first
version of the 1.3 release will be 1.3.0.  Any subsequent versions of
the release, for example 1.3.1, 1.3.2, etc., will generally contain only
bug fixes.  The next version to include new features will be the 1.4
release (1.4.0).

Within the [source code repository](https://github.com/folio-org/ldp)
there are two main branches:

* `master` is the branch that releases are made from.  It contains
  recently added features that have had some testing.

* `current` is for active development and tends to be unstable.  This is
  where new features are first added.

In addition there are numbered release branches based on `master` that
are used for bug fixes.
-->

### Building the software

To build the LDP software, download and unpack [the latest
release](https://github.com/folio-org/ldp/releases) and `cd` into the
unpacked directory.  Then:

```shell
$ mkdir build
$ cd build/
$ cmake ..
$ make
```
<!--
make install
-->

The compiled executable file `ldp` should appear in `ldp/build/src/`:

```shell
$ cd src/
$ ./ldp help
```


3\. Creating the LDP database
-----------------------------

Before using the LDP software, we need a database to load data into.
Two database users are also required: `ldpadmin`, an administrator
account, and `ldp`, a normal user of the database.  It is also a good
idea to restrict access permissions.

With PostgreSQL, for example, this can be done with:

```shell
$ createuser ldpadmin --username=<admin_user> --pwprompt
$ createuser ldp --username=<admin_user> --pwprompt
$ createdb ldp --username=<admin_user> --owner=ldpadmin
$ psql ldp --username=<admin_user> --single-transaction \
      --command="REVOKE ALL ON SCHEMA public FROM public;" \
      --command="GRANT ALL ON SCHEMA public TO ldpadmin;" \
      --command="GRANT USAGE ON SCHEMA public TO ldp;"
```

Additional command line options may be required to specify the database
host, port, etc.


4\. Configuration file
----------------------

The LDP software also needs a configuration file that looks something
like:

```
{
    "sources": {
        "folio": {
            "okapiURL": "https://folio-release-okapi.aws.indexdata.com",
            "okapiTenant": "diku",
            "okapiUser": "diku_admin",
            "okapiPassword": "(Okapi password here)",
            "extractDir": "/var/lib/ldp/extract/"
        }
    },
    "targets": {
        "ldpdemo": {
            "databaseName": "ldp",
            "databaseType": "postgresql",
            "databaseHost": "ldp.indexdata.com",
            "databasePort": "5432",
            "databaseUser": "ldpadmin",
            "databasePassword": "(database password here)"
        }
    }
}
```

The provided example file
[config_example.json](https://raw.githubusercontent.com/folio-org/ldp/master/config_example.json)
can be used as a template.

This file defines parameters for connecting to Okapi and to the LDP
database.  Please see the next section for how the parameters are used.

The LDP software looks for the configuration file in a location
specified by the `LDPCONFIG` environment variable, e.g. using the Bash
shell:

```shell
$ export LDPCONFIG=/etc/ldp/ldpconfig.json
```

The location also can be specified using the command line option
`--config`:

```shell
$ ldp load --config /etc/ldp/ldpconfig.json  ( etc. )
```


5\. Loading data into the database
----------------------------------

The LDP loader is intended to be run once per day, at a time of day when
usage is low, in order to refresh the database with new data from Okapi.

To extract data from Okapi and load them into the LDP database:
```shell
$ ldp load --source folio --target ldpdemo -v
```

The `load` command is used to load data.  The data are extracted from a
data "source" (Okapi) and loaded into a "target" (the LDP database).

The `--source` option specifies the name of a section under `sources` in
the LDP configuration file.  This section should provide connection
details for Okapi, as well as a temporary directory (`extractDir`) where
extracted files can be written.

The `--target` option works in the same way to specify a section under
`targets` in the configuration file that provides connection details for
the LDP database where data will be loaded.

The `-v` option enables verbose output.  For even more verbose output,
the `--debug` option can be used to see commands that are sent to the
database when loading data, among other details.  The `--debug` option
can generate extremely large output, and for this reason it is best used
when loading relatively small data sets.

Another option that is available to assist with debugging problems is
`--savetemps` (used together with `--unsafe`), which tells the loader
not to delete temporary files that store extracted data.  These files
are not anonymized and may contain personal data.


6\. Running the data loader in production
-----------------------------------------

The data loader is designed to be run automatically once per day, using
a job scheduler such as Cron.

While the data loader runs, the database is still available for users,
although query performance may be affected.  However, note that some of
the final stages of the loading process involve schema changes and can
interrupt any long-running queries that are executing at the same time.
For these reasons, it may be best to run the data loader at a time when
the database will not be used heavily.

If the data loading fails, the `ldp` process returns a non-zero exit
status.  It is recommended to check the exit status if the process is
being run automatically, in order to alert the administrator that data
loading was not completed.  In such cases the data loader generally
leaves the database unchanged, i.e. the database continues to reflect
the data from the previous successful data load.  Once the problem has
been addressed, simply run the data loader again.


7\. Anonymization of personal data
----------------------------------

By default, the LDP loader tries to "anonymize" personal data that it
extracts from Okapi by deleting values in user data from interface
`/users`, except for the following root-level fields:

* `id`
* `active`
* `type`
* `patronGroup`
* `enrollmentDate`
* `expirationDate`
* `meta`
* `proxyFor`
* `createdDate`
* `updatedDate`
* `metadata`
* `tags`

Note: We also plan to anonymize the user ID field, `id`, in the near
future.

There is no need to do anything to enable these anonymizations; they are
enabled unless the LDP loader is otherwise configured.

<!--
If it should be necessary to disable anonymization, this can be done by
setting the `disable_anonymization` parameter to `true` in the LDP
configuration file, within the section that defines connection details
for Okapi.

**WARNING:  Please note that this software does not provide a way to
anonymize the LDP database after personal data have been loaded into it.
This configuration parameter should be used only with great care.**

Turning on the `disable_anonymization` parameter prevents the LDP loader
from attempting to anonymize extracted data, so that all data provided
by Okapi will be loaded into the LDP database.
-->


8\. Loading data from files (for testing only)
----------------------------------------------

As an alternative to loading data with the `--source` option, source
data can be loaded directly from the file system for testing purposes,
using the `--unsafe` and `--sourcedir` options, e.g.:

```shell
$ ldp load --unsafe --sourcedir ldp-analytics/testdata/ --target ldpdemo
```

The loader expects the data files to have particular names, e.g.
`loans_0.json`; and when `--sourcedir` is used, it also looks for an
optional, accompanying file ending with the suffix, `_test.json`, e.g.
`loans_test.json`.  Data in these "test" files are loaded into the same
table as the files they accompany.  This is used for testing in query
development to combine extracted test data with additional static test
data.


9\. Disabling database TLS/SSL (for testing only)
-------------------------------------------------

For very preliminary testing with a database running on `localhost`, one
may desire to connect to the database without TLS/SSL.  Disabling
TLS/SSL is generally not recommended, because it may expose passwords
and library data transmitted to the database.  However, TLS/SSL can be
disabled using the `--unsafe` and `--nossl` options, e.g.:

```shell
$ ldp load --source folio --target ldpdemo --unsafe --nossl
```


10\. Reporting bugs and feature requests
----------------------------------------

Please use the [FOLIO Issue Tracker](https://issues.folio.org/) to
report a bug or feature request, by creating an issue in the "Library
Data Platform (LDP)" project.  Set the issue type to "Bug" or "New
Feature", and fill in the summary and description fields.  Please do not
set any other fields in the issue.

<!--
Please use the [issue tracker](https://github.com/folio-org/ldp/issues)
to report a bug or feature request.
-->


Further reading
---------------

[**Learn about using the LDP database in the User Guide > > >**](USER_GUIDE.md)

