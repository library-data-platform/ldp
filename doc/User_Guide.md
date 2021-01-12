LDP User Guide
==============

##### Contents  
[Overview](#overview)  
1\. [Data model](#1-data-model)  
2\. [JSON queries](#2-json-queries)  
3\. [Relational attributes vs. JSON](#3-relational-attributes-vs-json)  
4\. [Local tables](#4-local-tables)  
5\. [Historical data](#5-historical-data)  
6\. [Database views](#6-database-views)  
7\. [JSON arrays](#7-json-arrays)  
8\. [Community](#8-community)


Overview
--------

The Library Data Platform (LDP) is an open source platform for
analytics in libraries, offering a number of features:

* Query capability:  Ad hoc, cross-domain querying of data which are
  automatically extracted from source databases

* Compatibility:  Designed to work with popular tools for database
  access, analysis and visualization

* Data integration:  Offers a robust platform for combining data from
  beyond library systems

* Historical data:  Enables analyzing trends over time to gain insights
  about your library

* Scalability:  Provides an upgrade path that scales to virtually any
  amount of data that libraries can collect


1\. Data model
--------------

The primary LDP data model is a hybrid of relational and JSON schemas.
Each table contains JSON data in a relational attribute called `data`,
and a subset of the JSON fields is also stored in individual
relational attributes:

```sql
SELECT
    *
FROM
    circulation_loans
LIMIT 1;
```
```
id            | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
action        | checkedin
due_date      | 2020-02-17 08:43:15+00
item_id       | 459afaba-5b39-468d-9072-eb1685e0ddf4
item_status   | Available
loan_date     | 2020-02-03 08:43:15+00
renewal_count | 0
return_date   | 2020-03-01 11:35:12+00
user_id       | ab579dc3-219b-4f5b-8068-ab1c7a55c402
data          | {                                                          
              |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
              |     "action": "checkedin",                                 
              |     "dueDate": "2020-02-17T08:43:15.000+0000",             
              |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
              |     "itemStatus": "Available",                             
              |     "loanDate": "2020-02-03T08:43:15Z",                    
              |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
              |     "renewalCount": 0,                                     
              |     "returnDate": "2020-03-01T11:35:12Z",                  
              |     "status": {                                            
              |         "name": "Closed"                                   
              |     },                                                     
              |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
              | }
tenant_id     | 1
```

The relational attributes are provided to simplify writing queries,
and the JSON fields offer access to the complete source data.  One
additional attribute, `tenant_id`, is reserved for future use in
consortial environments.

The LDP software creates these tables, having extracted the data from
source databases.  It then updates the data from those sources once
per day, so that the LDP database reflects the state of the source
data as of sometime within the past 24 hours or so.


2\. JSON queries
----------------

To access the JSON fields, the function `json_extract_path_text()` can
be used to retrieve data from a path of up to five nested fields, for
example:

```sql
SELECT
    data
FROM
    circulation_loans
LIMIT 1;
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
     "dueDate": "2020-01-15T00:00:00Z",                
     "loanDate": "2020-01-01T00:00:00Z"                
 }                                                     
```

```sql
SELECT
    json_extract_path_text(data, 'status', 'name') AS status,
    count(*)
FROM
    circulation_loans
GROUP BY
    status;
```

```
 status | count
--------+--------
 Closed | 504407
 Open   |   1294
```

In this example, `json_extract_path_text(data, 'status', 'name')`
refers to the `name` field nested within the `status` field.

It is strongly recommended to use the function
`json_extract_path_text()` in particular because it is mostly
compatible with both PostgreSQL and Redshift, the two database systems
that are supported by LDP.  PostgreSQL is versatile, stable, and open
source, and Redshift offers fast queries on extremely large data.  By
using only functions that are compatible with both systems, you can
retain flexibility in the future to run either.


3\. Relational attributes vs. JSON
----------------------------------

An example of a query written entirely using JSON data:

```sql
SELECT
    json_extract_path_text(u.data, 'id') AS user_id,
    json_extract_path_text(g.data, 'group') AS "group"
FROM
    user_users AS u
    LEFT JOIN user_groups AS g
        ON json_extract_path_text(u.data, 'patronGroup') =
           json_extract_path_text(g.data, 'id')
LIMIT 5;
```

This can be written in a simpler form by using the relational
attributes rather than JSON fields:

```sql
SELECT
    u.id AS user_id,
    g.group
FROM
    user_users AS u
    LEFT JOIN user_groups AS g ON u.patron_group = g.id
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


4\. Local tables
----------------

The `local` schema is created by LDP as a common area in the database
where users can create or import their own data sets, including
storing the results of queries, e.g.:

```sql
CREATE TABLE local.loan_status AS
SELECT
    json_extract_path_text(data, 'status', 'name') AS status,
    count(*)
FROM
    circulation_loans
GROUP BY
    status;
```

This is also a good place to store tables containing intermediate
results, as a step-by-step way of building up complex queries.

After creating a local table that has many rows, it is a good idea to
create an index on each column that may be used either for filtering
in a `JOIN ... ON` or `WHERE` clause, or for sorting in an `ORDER BY`
clause:

```sql
CREATE TABLE local.loans_status AS
SELECT
    id,
    json_extract_path_text(data, 'status', 'name') AS status
FROM
    circulation_loans;

CREATE INDEX ON local.loans_status (id);

CREATE INDEX ON local.loans_status (status);
```

Also "vacuum" and "analyze" the table, which will help queries on the
table run faster:

```sql
VACUUM local.loans_status;

ANALYZE local.loans_status;
```


5\. Historical data
-------------------

### History tables

As mentioned earlier, the database contains a snapshot of the source
data as of the last update.  LDP also provides a schema called
`history` which stores all data that have been updated in the past,
including data that may no longer exist in the source.  Each table
normally has a corresponding history table, e.g.  the history table
for `circulation_loans` is `history.circulation_loans`.

This historical data capability can be used for gaining insights about
the library by analyzing trends over time.

History tables contain these attributes:

* `id` is the record ID.
* `data` is the source data, usually a JSON object.
* `updated` is the date and time when the data were updated.
* `tenant_id` is reserved for future use in consortial environments.

For example:

```sql
SELECT
    *
FROM
    history.circulation_loans
LIMIT 1;
```
```
id        | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
data      | {                                                          
          |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
          |     "action": "checkedin",                                 
          |     "dueDate": "2020-02-17T08:43:15.000+0000",             
          |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
          |     "itemStatus": "Available",                             
          |     "loanDate": "2020-02-03T08:43:15Z",                    
          |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
          |     "renewalCount": 0,                                     
          |     "returnDate": "2020-03-01T11:35:12Z",                  
          |     "status": {                                            
          |         "name": "Closed"                                   
          |     },                                                     
          |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
          | }
updated   | 2020-03-02 03:46:49.362606+00
tenant_id | 1
```

Unlike the main tables in which `id` is unique, the history tables can
accumulate many records with the same value for `id`.  Note also that
if a value in the source changes more than once during the interval
between two LDP updates, the history will only reflect the last of
those changes.

### Querying historical data

These are some basic examples that show data evolving over time.

For a high level view of all updated records, for example, in
`circulation_loans`:

```sql
SELECT
    updated,
    count(*)
FROM
    history.circulation_loans
GROUP BY
    updated
ORDER BY
    updated;
```

To view how a single record changes over time:

```sql
SELECT
    *
FROM
    history.circulation_loans
WHERE
    id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
ORDER BY
    updated;
```

The `SELECT` clause of this query can be modified to examine only
specific fields, e.g.:

```sql
SELECT
    json_extract_path_text(data, 'action'),
    json_extract_path_text(data, 'returnDate'),
    updated
FROM
    history.circulation_loans
WHERE
    id = '0bab56e5-1ab6-4ac2-afdf-8b2df0434378'
ORDER BY
    updated;
```

### Data cleaning

Since the source data schemas may evolve over time, the `data`
attribute in history tables does not necessarily have a single schema
that is consistent over an entire table.  As a result, querying
history tables may require "data cleaning."  A suggested first step
would be to select a subset of data within a time window, pulling out
JSON fields of interest into relational attributes, and storing this
result in a local table, e.g.:

```sql
CREATE TABLE local.loan_status_history AS
SELECT
    id,
    json_extract_path_text(data, 'itemStatus') AS item_status,
    updated
FROM
    history.circulation_loans
WHERE
    updated >= '2020-01-01'
    AND updated < '2021-01-01';
```

This will make it easier to examine the data to check for inconsistent
or missing values, update them, etc.  Note that in SQL, `''` and
`NULL` may look the same in the output of a `SELECT`, but they are
distinct values.


6\. Database views
------------------

The schema of source data can change over time, and LDP reflects these
changes when it updates data.  For this reason, LDP does not support
the use of database views.  LDP updates may fail to run if the
database contains views.  Instead of creating a view, use `CREATE
TABLE ... AS SELECT ...` to store a result set, as in the local schema
example above.


7\. JSON arrays
---------------

LDP does not yet support extracting arrays from JSON data.  However,
there is a workaround for PostgreSQL users.

A lateral join can be used with the function `json_array_elements()`
to convert the elements of a JSON array to a set of rows, one row per
array element.

For example, if the array elements are simple `varchar` strings:

```sql
CREATE TABLE local.example_array_simple AS
SELECT
    id AS instance_id,
    instance_format_ids.data #>> '{}' AS instance_format_id,
    instance_format_ids.ordinality
FROM
    inventory_instances
    CROSS JOIN LATERAL json_array_elements(json_extract_path(data, 'instanceFormatIds'))
        WITH ORDINALITY
        AS instance_format_ids (data);
```

If the array elements are JSON objects:

```sql
CREATE TABLE local.example_array_objects AS
SELECT
    id AS holdings_id,
    json_extract_path_text(notes.data, 'holdingsNoteTypeId')
        AS holdings_note_type_id,
    json_extract_path_text(notes.data, 'note') AS note,
    (json_extract_path_text(notes.data, 'staffOnly'))::boolean AS staff_only,
    notes.ordinality
FROM
    inventory_holdings
    CROSS JOIN LATERAL json_array_elements(json_extract_path(data, 'notes'))
        WITH ORDINALITY
        AS notes (data);
```


8\. Community
-------------

### Getting help

For questions or to share your work, please use the
[discussions](https://github.com/library-data-platform/ldp/discussions)
area.

### Bug reports

The [issue
tracker](https://github.com/library-data-platform/ldp/issues) can be
used to report a bug.


Further reading
---------------

[__Learn about configuring LDP in the Configuration Guide > > >__](Config_Guide.md)

[__Learn about installing and administering LDP in the Administrator Guide > > >__](Admin_Guide.md)

