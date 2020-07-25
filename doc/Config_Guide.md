LDP Configuration Guide
=======================

##### Contents  
1\. Scheduling full updates  
2\. Foreign keys  
Reference


1\. Scheduling full updates
---------------------------

Once per day, the LDP server runs a _full update_ which performs all
supported data updates.

Full updates can be scheduled at a preferred, recurring time of day by
setting `next_full_update` in table `ldpconfig.general`.  Note that
the timezone is part of the value.  For example:

```sql
UPDATE ldpconfig.general SET next_full_update = '2020-05-07 22:00:00Z';
```

This schedules the next full update on May 7, 2020 at 10:00 p.m. UTC.

Also ensure that `full_update_enabled` is set to `TRUE`.

A full update may take several hours for a large library.  During this
time, the database generally remains available to users, but query
performance may be affected.  Also, some stages of the update process
involve schema changes and could interrupt any long-running queries
that are executing at the same time.  For these reasons, it is best to
run full updates at a time when the database will be least heavily
used.


2\. Foreign keys
----------------

### Detecting foreign keys

LDP can infer foreign key relationships in data.  To enable this
feature:

```sql
UPDATE ldpconfig.general SET detect_foreign_keys = TRUE;
```

This will cause the foreign key analysis to run directly after every
full update.  The results, in the form of suggested foreign key
constraints, are placed in the table
`ldpsystem.suggested_foreign_keys`.  The data in this table can be
used to create foreign key constraints, as described in the next
section.

### Foreign key constraints

After a full update LDP can optionally create foreign key constraints,
which are defined in the table `ldpconfig.foreign_keys`.  This table
has exactly the same schema as table
`ldpsystem.suggested_foreign_keys` described in the previous section,
which allows the detected foreign keys to be easily used as a starting
point for configuring constraints:

```sql
INSERT INTO ldpconfig.foreign_keys
    SELECT * FROM ldpsystem.suggested_foreign_keys;
```

These constraints have no effect unless they are activated by setting
`enable_foreign_key_warnings` or `force_foreign_key_constraints` (or
both) in the table `ldpconfig.general`:

The `enable_foreign_key_warnings` does not create any constraints, but
it writes referential integrity warnings to `ldpsystem.log` as though
the constraints were to be created.

The `force_foreign_key_constraints` creates foreign key constraints.
In order to do this, it deletes rows having foreign keys that are not
present in referenced tables.

Both of these configuration values take effect after every full
update.


Reference
---------

### Table: ldpconfig.general

* `enable_full_updates` (BOOLEAN) turns on daily full updates when set
  to `TRUE`.  No full updates are performed if it is set to `FALSE`.

* `next_full_update` (TIMESTAMP WITH TIME ZONE) is the date and time
  of the next full update.  Once the full update begins, this value is
  automatically incremented to the next day at the same time.

* `detect_foreign_keys` (BOOLEAN) enables detection of foreign key
  relationships between tables in the `public` schema.  If enabled,
  the analysis runs after every full update and the table
  `ldpsystem.suggested_foreign_keys` contains the results; otherwise
  the table is cleared.  The table schema is (`enable_constraint`,
  `referencing_table`, `referencing_column`, `referenced_table`,
  `referenced_column`).  Each listed pair of (`referencing table`,
  `referencing column`) appears in one or more rows that contain
  "candidate" pairs of (`referenced table`, `referenced column`).  If
  the referencing table and column have only one candidate, then the
  attribute `enable_constraint` is set to `TRUE`; otherwise it is
  `FALSE`.

* `enable_foreign_key_warnings` (BOOLEAN) enables logging of
  referential integrity warnings to `ldpsystem.log` that would be
  generated if the database were to enforce the foreign key
  constraints specified in `ldpconfig.foreign_keys` where
  `enable_constraint` is set to `TRUE`.

* `force_foreign_key_constraints` (BOOLEAN) enables creation of
  foreign key constraints specified in the table
  `ldpconfig.foreign_keys` where `enable_constraint` is set to `TRUE`.
  This process deletes any rows having the specified foreign keys that
  are not present in referenced tables.  The deleted rows are the same
  ones logged as referential integrity warnings if
  `enable_foreign_key_warnings` has been set.

* `disable_anonymization` (BOOLEAN) when set to `TRUE`, disables
  anonymization of personal data.  Please read the section on "Data
  privacy" in the [Administrator Guide](Admin_Guide.md) before
  changing this setting.  As a safety precaution, the configuration
  setting `disable_anonymization` in `ldpconf.json` also must be set
  by the LDP system administrator.


Further reading
---------------

[__Learn about installing and administering LDP in the
Administrator Guide > > >__](Admin_Guide.md)

[__Learn about using the LDP database in the
User Guide > > >__](User_Guide.md)

