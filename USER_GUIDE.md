LDP User Guide
==============

##### Contents  
Overview  
1\. Data model  
2\. JSON queries  
3\. Relational attributes vs. JSON  
4\. Local schemas  
5\. Historical data  
6\. Important note on database views  
7\. Community


Overview
--------

The Library Data Platform (LDP) is an open source platform for reporting
and analytics in libraries, offering a number of features:

* Query capability:  Ad hoc, cross-domain querying of data which are
  automatically extracted from Okapi-based microservices

* Compatibility:  Designed to work with popular graphical database
  clients like Tableau, Microsoft Access, and DBeaver, as well as data
  analysis software such as R

* Data integration:  Offers a robust platform for combining data from
  beyond library systems

* Historical data:  Enables analyzing trends over time to gain strategic
  insights about your library

* Scalability:  Provides an upgrade path that scales to virtually any
  amount of data that libraries can collect

The LDP is available now in "pre-release" versions for testing purposes,
with version 1.0 expected in mid-2020.  This documentation covers the
LDP as it exists at the present time.


1\. Data model
--------------

The primary LDP data model is a hybrid of relational and JSON schemas.
Each table contains JSON data in a relational attribute called `data`,
and a subset of the JSON fields is also stored in individual relational
attributes:

```sql
SELECT * FROM circulation_loans LIMIT 1;
```
```
row_id        | 5326
id            | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
action        | checkedin
due_date      | 2017-02-17 08:43:15+00
item_id       | 459afaba-5b39-468d-9072-eb1685e0ddf4
item_status   | Available
loan_date     | 2017-02-03 08:43:15+00
proxy_user_id | a13dda6b-51ce-4543-be9c-eff894c0c2d0
renewal_count | 0
return_date   | 2017-03-01 11:35:12+00
user_id       | ab579dc3-219b-4f5b-8068-ab1c7a55c402
data          | {                                                          
              |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
              |     "action": "checkedin",                                 
              |     "dueDate": "2017-02-17T08:43:15.000+0000",             
              |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
              |     "itemStatus": "Available",                             
              |     "loanDate": "2017-02-03T08:43:15Z",                    
              |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
              |     "renewalCount": 0,                                     
              |     "returnDate": "2017-03-01T11:35:12Z",                  
              |     "status": {                                            
              |         "name": "Closed"                                   
              |     },                                                     
              |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
              | }
tenant_id     | 1
```

The relational attributes are provided to simplify writing queries and to
improve query performance.  The JSON fields offer access to the complete
extracted source data.

In addition, the attribute `row_id` is an LDP-specific key, and
`tenant_id` is reserved for future use in consortial reporting.

The data in these tables are extracted from Okapi-based APIs and loaded
into the database by the LDP data loader.  The data loader typically
runs once per day, and so the LDP database reflects the state of the
source data as of sometime within the past 24 hours or so.

<!--
A table called `ldp_catalog.table_updates` records when each data
table was last updated.
-->


2\. JSON queries
----------------

To access the JSON fields, it is recommended to use the built in function
`json_extract_path_text()` to retrieve data from a path of up to five nested
JSON fields, for example:

```sql
SELECT data FROM circulation_loans LIMIT 1;
```

```
                         data                          
-------------------------------------------------------
 {                                                     
     "id": "87598cd2-4989-4191-aaf0-c8fbb5552a06",     
     "action": "checkedout",                           
     "itemId": "575d5eec-ad28-472f-ba79-09f8cf5539a3", 
     "status": {                                       
         "name": "Open"                                
     },                                                
     "userId": "79dffc9d-be22-462e-b46f-78ca88babe91", 
     "dueDate": "2018-01-15T00:00:00Z",                
     "loanDate": "2018-01-01T00:00:00Z"                
 }                                                     
```

```sql
SELECT json_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM circulation_loans
    GROUP BY status;
```

```
 status | count
--------+--------
 Closed | 504407
 Open   |   1294
```

In this example, `json_extract_path_text(data, 'status', 'name')` refers to
the `name` field nested within the `status` field.


3\. Relational attributes vs. JSON
----------------------------------

An example of a query written entirely using JSON data:

```sql
SELECT json_extract_path_text(users.data, 'id') AS user_id,
       json_extract_path_text(groups.data, 'group') AS group
    FROM user_users
        LEFT JOIN user_groups
            ON json_extract_path_text(users.data, 'patronGroup') =
               json_extract_path_text(groups.data, 'id')
    LIMIT 5;
```

This can be written in a simpler form by using the relational attributes
rather than JSON fields:

```sql
SELECT users.id AS user_id,
       groups.group
    FROM user_users
        LEFT JOIN user_groups
	    ON users.patron_group = groups.id
    LIMIT 5;
```

```
               user_id                |    group
--------------------------------------+---------------
 429510c8-0b50-4ec5-b4ca-462278c0677b | staff
 af91674a-6468-4d5f-b037-28ebbf02f051 | undergraduate
 0736f1a1-6a19-4f13-ab3b-50a4915dcc4a | undergraduate
 37274158-5e38-4852-93c7-359bfdd76892 | undergraduate
 2c2864b1-58cb-4752-ae8e-3ce7bc9a3be4 | faculty
```


4\. Local schemas
-----------------

The `local` schema is created by the LDP loader as a common area in the
database where reporting users can create or import their own data sets,
including storing the results of queries, e.g.:

```sql
CREATE TABLE local.loan_status AS
SELECT json_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM circulation_loans
    GROUP BY status;
```

The `ldp` user is granted database permissions to create tables in the
`local` schema.

If additional local schemas are desired, it is recommended that the new
schema names be prefixed with `local_` or `l_` to avoid future naming
collisions with the LDP.


5\. Historical data
-------------------

### Overview

As mentioned earlier, the LDP database reflects the state of the
source data as of the last time the LDP data loader was run.  The
loader also maintains another schema called `history` which stores all
data that have been loaded in the past, including data that no longer
exist in the source database.  Each table normally has a corresponding
history table, e.g. the history table for `circulation_loans` is
`history.circulation_loans`.

This historical data capability is designed for gaining insights about
the library by analyzing trends over time.

History tables contain these attributes:

* `row_id` is an LDP-specific key.

* `id` is the record ID.

* `data` is the source data, usually a JSON object.

* `updated` is the date and time when the data were loaded.

* `tenant_id` is reserved for future use in consortial reporting.

For example:

```sql
SELECT * FROM history.circulation_loans LIMIT 1;
```
```
row_id    | 29201
id        | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
data      | {                                                          
          |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
          |     "action": "checkedin",                                 
          |     "dueDate": "2017-02-17T08:43:15.000+0000",             
          |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
          |     "itemStatus": "Available",                             
          |     "loanDate": "2017-02-03T08:43:15Z",                    
          |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
          |     "renewalCount": 0,                                     
          |     "returnDate": "2017-03-01T11:35:12Z",                  
          |     "status": {                                            
          |         "name": "Closed"                                   
          |     },                                                     
          |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
          | }
updated   | 2019-09-06 03:46:49.362606+00
tenant_id | 1
```

Unlike the main LDP tables in which `id` is unique, the history tables
can accumulate many records with the same value for `id`.  Note also
that if a value in the source database changes more than once during
the interval between any two runs of the LDP loader, the LDP history
will only reflect the last of those changes.

### Querying historical data

These are some basic examples that show data evolving over time.

For a high level view of all updated records, for example, in
circulation_loans:

```sql
SELECT updated,
       count(*)
    FROM history.circulation_loans
    GROUP BY updated
    ORDER BY updated;
```

To view how a single record can change over time, using the record ID
above:

```sql
SELECT *
    FROM history.circulation_loans
    WHERE id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
    ORDER BY updated;
```

The `SELECT` clause of this query can be modified to examine only
specific fields, e.g.:

```sql
SELECT json_extract_path_text(data, 'action'),
       json_extract_path_text(data, 'returnDate'),
       updated
    FROM history.circulation_loans
    WHERE id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
    ORDER BY updated;
```

### Data cleaning

Since the source data schemas may evolve over time, the `data` attribute
in history tables does not necessarily have a single schema that is
consistent over an entire table.  As a result, reporting on history
tables may require some "data cleaning" as preparation before the data
can be queried accurately.  A suggested first step could be to select a
subset of data within a time window, pulling out JSON fields of interest
into relational attributes, and storing this result in a local table,
e.g.:

```sql
CREATE TABLE local.loan_status_history AS
SELECT id,
       json_extract_path_text(data, 'itemStatus') AS item_status,
       updated
    FROM history.circulation_loans
    WHERE updated BETWEEN '2019-01-01' AND '2019-12-31';
```

This will make it easier to examine the data to check for inconsistent
or missing values, update them, etc.  Note that in SQL, `''` and `NULL`
may look the same in the output of a `SELECT`, but they are distinct
values.


6\. Important note on database views
------------------------------------

The schema of source data can change over time, and the LDP reflects
these changes when it refreshes its data.  For this reason, the LDP
cannot support the use of database views.  The LDP loader may fail to
run if the database contains views.  Instead of creating a view, use
`CREATE TABLE ... AS SELECT ...` to store a result set, as in the local
schema example above.

Reporting users should be aware of schema changes in advance, in order
to be able to update queries and to prepare to recreate local result
sets if needed.


7\. Community
-------------

### Mailing lists

Mailing lists for the LDP software are hosted on FOLIO's discussion
site:

* [`ldp-announce`](https://discuss.folio.org/c/ldp) is a low volume
  announcement list.

* [`ldp-users`](https://discuss.folio.org/c/ldp/ldp-users) is for
  general usage and querying of the LDP database.

* [`ldp-sysadmin`](https://discuss.folio.org/c/ldp/ldp-sysadmin) is
  for system administration of the LDP software.

FOLIO's [Slack organization](https://slack-invitation.folio.org/) also
contains LDP-related channels.

### Bugs and feature requests

Please use the [issue tracker](https://github.com/folio-org/ldp/issues)
to report a bug or feature request.

<!--
Please use the [FOLIO Issue Tracker](https://issues.folio.org/) to
report a bug or feature request, by creating an issue in the "Library
Data Platform (LDP)" project.  Set the issue type to "Bug" or "New
Feature", and fill in the summary and description fields.  Please do not
set any other fields in the issue.
-->

### FOLIO reporting

Librarians and developers in the FOLIO community are building a
collection of LDP-based queries to provide reporting capabilities for
FOLIO libraries.  This effort is organized around the
[ldp-analytics](https://github.com/folio-org/ldp-analytics)
repository.


Further reading
---------------

[__Learn about installing and administering the LDP in the Admin Guide > > >__](ADMIN_GUIDE.md)

